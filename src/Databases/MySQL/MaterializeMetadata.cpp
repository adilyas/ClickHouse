#include <Databases/MySQL/MaterializeMetadata.h>

#if USE_MYSQL

#include <DataTypes/DataTypeString.h>
#include <DataTypes/DataTypesNumber.h>
#include <Databases/MySQL/MySQLUtils.h>
#include <Formats/MySQLBlockInputStream.h>
#include <IO/ReadBufferFromFile.h>
#include <IO/WriteBufferFromFile.h>
#include <Poco/File.h>
#include <Common/quoteString.h>
#include <IO/ReadHelpers.h>
#include <IO/WriteHelpers.h>

namespace DB
{

namespace ErrorCodes
{
    extern const int LOGICAL_ERROR;
}

void MaterializeMetadata::fetchMasterStatus(mysqlxx::PoolWithFailover::Entry & connection)
{
    Block header{
        {std::make_shared<DataTypeString>(), "File"},
        {std::make_shared<DataTypeUInt64>(), "Position"},
        {std::make_shared<DataTypeString>(), "Binlog_Do_DB"},
        {std::make_shared<DataTypeString>(), "Binlog_Ignore_DB"},
        {std::make_shared<DataTypeString>(), "Executed_Gtid_Set"},
    };

    MySQLBlockInputStream input(connection, "SHOW MASTER STATUS;", header, DEFAULT_BLOCK_SIZE);
    Block master_status = input.read();

    if (!master_status || master_status.rows() != 1)
        throw Exception("Unable to get master status from MySQL.", ErrorCodes::LOGICAL_ERROR);

    data_version = 1;
    binlog_file = (*master_status.getByPosition(0).column)[0].safeGet<String>();
    binlog_position = (*master_status.getByPosition(1).column)[0].safeGet<UInt64>();
    binlog_do_db = (*master_status.getByPosition(2).column)[0].safeGet<String>();
    binlog_ignore_db = (*master_status.getByPosition(3).column)[0].safeGet<String>();
    executed_gtid_set = (*master_status.getByPosition(4).column)[0].safeGet<String>();
}

Block MaterializeMetadata::getShowMasterLogHeader() const
{
    if (startsWith(mysql_version, "5."))
    {
        return Block {
            {std::make_shared<DataTypeString>(), "Log_name"},
            {std::make_shared<DataTypeUInt64>(), "File_size"}
        };
    }

    return Block {
        {std::make_shared<DataTypeString>(), "Log_name"},
        {std::make_shared<DataTypeUInt64>(), "File_size"},
        {std::make_shared<DataTypeString>(), "Encrypted"}
    };
}

bool MaterializeMetadata::checkBinlogFileExists(mysqlxx::PoolWithFailover::Entry & connection) const
{
    MySQLBlockInputStream input(connection, "SHOW MASTER LOGS", getShowMasterLogHeader(), DEFAULT_BLOCK_SIZE);

    while (Block block = input.read())
    {
        for (size_t index = 0; index < block.rows(); ++index)
        {
            const auto & log_name = (*block.getByPosition(0).column)[index].safeGet<String>();
            if (log_name == binlog_file)
                return true;
        }
    }
    return false;
}

void MaterializeMetadata::commitMetadata(const std::function<void()> & function, const String & persistent_tmp_path)
{
    try
    {
        function();

        Poco::File(persistent_tmp_path).renameTo(persistent_path);
    }
    catch (...)
    {
        Poco::File(persistent_tmp_path).remove();
        throw;
    }
}

void MaterializeMetadata::transaction(const MySQLReplication::Position & position, const std::function<void()> & fun)
{
    if (!is_initialized)
        throw Exception("Initialize metadata before calling transaction.", ErrorCodes::LOGICAL_ERROR);

    binlog_file = position.binlog_name;
    binlog_position = position.binlog_pos;
    executed_gtid_set = position.gtid_sets.toString();

    String persistent_tmp_path = persistent_path + ".tmp";

    {
        WriteBufferFromFile out(persistent_tmp_path, DBMS_DEFAULT_BUFFER_SIZE, O_WRONLY | O_TRUNC | O_CREAT | O_EXCL);

        /// TSV format metadata file.
        writeString("Version:\t" + toString(meta_version), out);
        writeString("\nBinlog File:\t" + binlog_file, out);
        writeString("\nExecuted GTID:\t" + executed_gtid_set, out);
        writeString("\nBinlog Position:\t" + toString(binlog_position), out);
        writeString("\nData Version:\t" + toString(data_version), out);

        out.next();
        out.sync();
        out.close();
    }

    commitMetadata(std::move(fun), persistent_tmp_path);
}

void MaterializeMetadata::fetchMetadata(mysqlxx::PoolWithFailover::Entry & connection)
{
    bool locked_tables = false;
    try {
        connection->query("FLUSH TABLES;").execute();
        connection->query("FLUSH TABLES WITH READ LOCK;").execute();

        locked_tables = true;
        fetchMasterStatus(connection);
        connection->query("UNLOCK TABLES;").execute();
    }
    catch (...) {
        if (locked_tables)
            connection->query("UNLOCK TABLES;").execute();

        throw;
    }
}

bool MaterializeMetadata::tryInitFromFile(mysqlxx::PoolWithFailover::Entry & connection)
{
    if (!Poco::File(persistent_path).exists()) {
        ReadBufferFromFile in(persistent_path, DBMS_DEFAULT_BUFFER_SIZE);
        assertString("Version:\t" + toString(meta_version), in);
        assertString("\nBinlog File:\t", in);
        readString(binlog_file, in);
        assertString("\nExecuted GTID:\t", in);
        readString(executed_gtid_set, in);
        assertString("\nBinlog Position:\t", in);
        readIntText(binlog_position, in);
        assertString("\nData Version:\t", in);
        readIntText(data_version, in);

        if (checkBinlogFileExists(connection))
            return true;
    }

    return false;
}

MaterializeMetadata::MaterializeMetadata(
    mysqlxx::PoolWithFailover::Entry & connection,
    const String & path_,
    const String & mysql_version_)
    : persistent_path(path_)
    , mysql_version(mysql_version_)
    , is_initialized(true)
{
    if (tryInitFromFile(connection)) {
        return;
    }

    fetchMetadata(connection); // this will be overriden in case of database
}

MaterializeMetadata::MaterializeMetadata()
    : is_initialized(false)
{
}

}

#endif
