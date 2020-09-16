#pragma once

#include "config_core.h"

#if USE_MYSQL

#include <mysqlxx/Pool.h>
#include <Core/MySQL/MySQLClient.h>
#include <Databases/IDatabase.h>
#include <Databases/MySQL/MaterializeMySQLSettings.h>
#include <Databases/MySQL/MaterializeMySQLSyncThread.h>

namespace DB
{

/** Real-time pull table structure and data from remote MySQL
 *
 *  All table structure and data will be written to the local file system
 */
class DatabaseMaterializeMySQL : public IDatabase
{
public:
    DatabaseMaterializeMySQL(
        const Context & context,
        const String & database_name_,
        const String & metadata_path_,
        const IAST * database_engine_define_,
        const String & mysql_database_name_,
        mysqlxx::Pool && pool_,
        MySQLClient && client_,
        std::unique_ptr<MaterializeMySQLSettings> settings_);

    void rethrowExceptionIfNeed() const;

    void setException(const std::exception_ptr & exception);
protected:
    const Context & global_context;

    ASTPtr engine_define;
    DatabasePtr nested_database;
    String mysql_database_name;
    std::unique_ptr<MaterializeMySQLSettings> settings;

    mutable mysqlxx::Pool pool;
    MySQLClient client;
    Poco::Logger * log;
    MaterializeMetadataPtr materialize_metadata;
    MaterializeMySQLSyncThreadPtr materialize_thread;

    std::exception_ptr exception;

    String query_prefix;

public:
    String getEngineName() const override { return "MaterializeMySQL"; }

    ASTPtr getCreateDatabaseQuery() const override;

    std::vector<String> fetchTablesInDB(const mysqlxx::PoolWithFailover::Entry & connection);

    std::unordered_map<String, String> fetchTablesCreateQuery(const mysqlxx::PoolWithFailover::Entry & connection);

    void cleanOutdatedTables(const Context & context);

    void executeDumpQueries(
        mysqlxx::Pool::Entry & connection,
        const Context & context,
        std::unordered_map<String, String> tables_dump_queries);

    MaterializeMetadataPtr tryDumpTablesData(
        mysqlxx::PoolWithFailover::Entry & connection,
        bool & opened_transaction);

    MaterializeMetadataPtr dumpTablesData();

    void loadStoredObjects(Context & context, bool has_force_restore_data_flag, bool force_attach) override;

    void shutdown() override;

    bool empty() const override;

    String getDataPath() const override;

    String getTableDataPath(const String & table_name) const override;

    String getTableDataPath(const ASTCreateQuery & query) const override;

    UUID tryGetTableUUID(const String & table_name) const override;

    void createTable(const Context & context, const String & name, const StoragePtr & table, const ASTPtr & query) override;

    void dropTable(const Context & context, const String & name, bool no_delay) override;

    void attachTable(const String & name, const StoragePtr & table, const String & relative_table_path) override;

    StoragePtr detachTable(const String & name) override;

    void renameTable(const Context & context, const String & name, IDatabase & to_database, const String & to_name, bool exchange, bool dictionary) override;

    void alterTable(const Context & context, const StorageID & table_id, const StorageInMemoryMetadata & metadata) override;

    time_t getObjectMetadataModificationTime(const String & name) const override;

    String getMetadataPath() const override;

    String getObjectMetadataPath(const String & table_name) const override;

    bool shouldBeEmptyOnDetach() const override;

    void drop(const Context & context) override;

    bool isTableExist(const String & name, const Context & context) const override;

    StoragePtr tryGetTable(const String & name, const Context & context) const override;

    DatabaseTablesIteratorPtr getTablesIterator(const Context & context, const FilterByNameFunction & filter_by_table_name) override;
};

}

#endif
