#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <common/types.h>


namespace DB
{

/// Data types for representing elementary values from a database in RAM.

struct Null {};

/// @note Except explicitly described you should not assume on TypeIndex numbers and/or their orders in this enum.
enum class TypeIndex
{
    Nothing = 0,
    UInt8,
    UInt16,
    UInt32,
    UInt64,
    UInt128,
    bUInt256,
    Int8,
    Int16,
    Int32,
    Int64,
    Int128,
    bInt256,
    Float32,
    Float64,
    Date,
    DateTime,
    DateTime64,
    String,
    FixedString,
    Enum8,
    Enum16,
    Decimal32,
    Decimal64,
    Decimal128,
    Decimal256,
    UUID,
    Array,
    Tuple,
    Set,
    Interval,
    Nullable,
    Function,
    AggregateFunction,
    LowCardinality,
};

/// defined in common/types.h
using UInt8 = ::UInt8;
using UInt16 = ::UInt16;
using UInt32 = ::UInt32;
using UInt64 = ::UInt64;
using bUInt256 = ::bUInt256;

using Int8 = ::Int8;
using Int16 = ::Int16;
using Int32 = ::Int32;
using Int64 = ::Int64;
using Int128 = ::Int128;
using bInt256 = ::bInt256;

using Float32 = float;
using Float64 = double;

using String = std::string;


/** Note that for types not used in DB, IsNumber is false.
  */
template <typename T> constexpr bool IsNumber = false;

template <> inline constexpr bool IsNumber<UInt8> = true;
template <> inline constexpr bool IsNumber<UInt16> = true;
template <> inline constexpr bool IsNumber<UInt32> = true;
template <> inline constexpr bool IsNumber<UInt64> = true;
template <> inline constexpr bool IsNumber<bUInt256> = true;
template <> inline constexpr bool IsNumber<Int8> = true;
template <> inline constexpr bool IsNumber<Int16> = true;
template <> inline constexpr bool IsNumber<Int32> = true;
template <> inline constexpr bool IsNumber<Int64> = true;
template <> inline constexpr bool IsNumber<Int128> = true;
template <> inline constexpr bool IsNumber<bInt256> = true;
template <> inline constexpr bool IsNumber<Float32> = true;
template <> inline constexpr bool IsNumber<Float64> = true;

template <typename T> struct TypeName;

template <> struct TypeName<UInt8>   { static constexpr const char * get() { return "UInt8";   } };
template <> struct TypeName<UInt16>  { static constexpr const char * get() { return "UInt16";  } };
template <> struct TypeName<UInt32>  { static constexpr const char * get() { return "UInt32";  } };
template <> struct TypeName<UInt64>  { static constexpr const char * get() { return "UInt64";  } };
template <> struct TypeName<bUInt256> { static constexpr const char * get() { return "UInt256"; } };
template <> struct TypeName<Int8>    { static constexpr const char * get() { return "Int8";    } };
template <> struct TypeName<Int16>   { static constexpr const char * get() { return "Int16";   } };
template <> struct TypeName<Int32>   { static constexpr const char * get() { return "Int32";   } };
template <> struct TypeName<Int64>   { static constexpr const char * get() { return "Int64";   } };
template <> struct TypeName<Int128>  { static constexpr const char * get() { return "Int128";  } };
template <> struct TypeName<bInt256> { static constexpr const char * get() { return "Int256";  } };
template <> struct TypeName<Float32> { static constexpr const char * get() { return "Float32"; } };
template <> struct TypeName<Float64> { static constexpr const char * get() { return "Float64"; } };
template <> struct TypeName<String>  { static constexpr const char * get() { return "String";  } };

template <typename T> struct TypeId;
template <> struct TypeId<UInt8>    { static constexpr const TypeIndex value = TypeIndex::UInt8;  };
template <> struct TypeId<UInt16>   { static constexpr const TypeIndex value = TypeIndex::UInt16;  };
template <> struct TypeId<UInt32>   { static constexpr const TypeIndex value = TypeIndex::UInt32;  };
template <> struct TypeId<UInt64>   { static constexpr const TypeIndex value = TypeIndex::UInt64;  };
template <> struct TypeId<bUInt256> { static constexpr const TypeIndex value = TypeIndex::bUInt256; };
template <> struct TypeId<Int8>     { static constexpr const TypeIndex value = TypeIndex::Int8;  };
template <> struct TypeId<Int16>    { static constexpr const TypeIndex value = TypeIndex::Int16; };
template <> struct TypeId<Int32>    { static constexpr const TypeIndex value = TypeIndex::Int32; };
template <> struct TypeId<Int64>    { static constexpr const TypeIndex value = TypeIndex::Int64; };
template <> struct TypeId<Int128>   { static constexpr const TypeIndex value = TypeIndex::Int128; };
template <> struct TypeId<bInt256>  { static constexpr const TypeIndex value = TypeIndex::bInt256; };
template <> struct TypeId<Float32>  { static constexpr const TypeIndex value = TypeIndex::Float32;  };
template <> struct TypeId<Float64>  { static constexpr const TypeIndex value = TypeIndex::Float64;  };

/// Not a data type in database, defined just for convenience.
using Strings = std::vector<String>;

/// Own FieldType for Decimal.
/// It is only a "storage" for decimal. To perform operations, you also have to provide a scale (number of digits after point).
template <typename T>
struct Decimal
{
    using NativeType = T;

    Decimal() = default;
    Decimal(Decimal<T> &&) = default;
    Decimal(const Decimal<T> &) = default;

    Decimal(const T & value_)
    :   value(value_)
    {}

    template <typename U>
    Decimal(const Decimal<U> & x)
    :   value(x.value)
    {}

    constexpr Decimal<T> & operator = (Decimal<T> &&) = default;
    constexpr Decimal<T> & operator = (const Decimal<T> &) = default;

    operator T () const { return value; }

    template <typename U>
    U convertTo()
    {
        /// no IsDecimalNumber defined yet
        if constexpr (std::is_same_v<U, Decimal<Int32>> ||
                      std::is_same_v<U, Decimal<Int64>> ||
                      std::is_same_v<U, Decimal<Int128>> ||
                      std::is_same_v<U, Decimal<bInt256>>)
        {
            return convertTo<typename U::NativeType>();
        }
        else if constexpr (is_big_int_v<NativeType>)
        {
            return value. template convert_to<U>();
        }
        else
            return static_cast<U>(value);
    }

    const Decimal<T> & operator += (const T & x) { value += x; return *this; }
    const Decimal<T> & operator -= (const T & x) { value -= x; return *this; }
    const Decimal<T> & operator *= (const T & x) { value *= x; return *this; }
    const Decimal<T> & operator /= (const T & x) { value /= x; return *this; }
    const Decimal<T> & operator %= (const T & x) { value %= x; return *this; }

    T value;
};

template <typename T> inline bool operator< (const Decimal<T> & x, const Decimal<T> & y) { return x.value < y.value; }
template <typename T> inline bool operator> (const Decimal<T> & x, const Decimal<T> & y) { return x.value > y.value; }
template <typename T> inline bool operator== (const Decimal<T> & x, const Decimal<T> & y) { return x.value == y.value; }
template <typename T> inline bool operator!= (const Decimal<T> & x, const Decimal<T> & y) { return x.value != y.value; }

template <typename T> inline Decimal<T> operator+ (const Decimal<T> & x, const Decimal<T> & y) { return x.value + y.value; }
template <typename T> inline Decimal<T> operator- (const Decimal<T> & x, const Decimal<T> & y) { return x.value - y.value; }
template <typename T> inline Decimal<T> operator* (const Decimal<T> & x, const Decimal<T> & y) { return x.value * y.value; }
template <typename T> inline Decimal<T> operator/ (const Decimal<T> & x, const Decimal<T> & y) { return x.value / y.value; }
template <typename T> inline Decimal<T> operator- (const Decimal<T> & x) { return -x.value; }

using Decimal32 = Decimal<Int32>;
using Decimal64 = Decimal<Int64>;
using Decimal128 = Decimal<Int128>;
using Decimal256 = Decimal<bInt256>;

using DateTime64 = Decimal64;

template <> struct TypeName<Decimal32>   { static constexpr const char * get() { return "Decimal32";   } };
template <> struct TypeName<Decimal64>   { static constexpr const char * get() { return "Decimal64";   } };
template <> struct TypeName<Decimal128>  { static constexpr const char * get() { return "Decimal128";  } };
template <> struct TypeName<Decimal256>  { static constexpr const char * get() { return "Decimal256";  } };

template <> struct TypeId<Decimal32>    { static constexpr const TypeIndex value = TypeIndex::Decimal32; };
template <> struct TypeId<Decimal64>    { static constexpr const TypeIndex value = TypeIndex::Decimal64; };
template <> struct TypeId<Decimal128>   { static constexpr const TypeIndex value = TypeIndex::Decimal128; };
template <> struct TypeId<Decimal256>   { static constexpr const TypeIndex value = TypeIndex::Decimal256; };

template <typename T> constexpr bool IsDecimalNumber = false;
template <> inline constexpr bool IsDecimalNumber<Decimal32> = true;
template <> inline constexpr bool IsDecimalNumber<Decimal64> = true;
template <> inline constexpr bool IsDecimalNumber<Decimal128> = true;
template <> inline constexpr bool IsDecimalNumber<Decimal256> = true;

template <typename T> struct NativeType { using Type = T; };
template <> struct NativeType<Decimal32> { using Type = Int32; };
template <> struct NativeType<Decimal64> { using Type = Int64; };
template <> struct NativeType<Decimal128> { using Type = Int128; };
template <> struct NativeType<Decimal256> { using Type = bInt256; };

template <typename T> constexpr bool OverBigInt = false;
template <> inline constexpr bool OverBigInt<bInt256> = true;
template <> inline constexpr bool OverBigInt<bUInt256> = true;
template <> inline constexpr bool OverBigInt<Decimal256> = true;

inline constexpr const char * getTypeName(TypeIndex idx)
{
    switch (idx)
    {
        case TypeIndex::Nothing:    return "Nothing";
        case TypeIndex::UInt8:      return TypeName<UInt8>::get();
        case TypeIndex::UInt16:     return TypeName<UInt16>::get();
        case TypeIndex::UInt32:     return TypeName<UInt32>::get();
        case TypeIndex::UInt64:     return TypeName<UInt64>::get();
        case TypeIndex::UInt128:    return "UInt128";
        case TypeIndex::bUInt256:   return TypeName<bUInt256>::get();
        case TypeIndex::Int8:       return TypeName<Int8>::get();
        case TypeIndex::Int16:      return TypeName<Int16>::get();
        case TypeIndex::Int32:      return TypeName<Int32>::get();
        case TypeIndex::Int64:      return TypeName<Int64>::get();
        case TypeIndex::Int128:     return TypeName<Int128>::get();
        case TypeIndex::bInt256:    return TypeName<bInt256>::get();
        case TypeIndex::Float32:    return TypeName<Float32>::get();
        case TypeIndex::Float64:    return TypeName<Float64>::get();
        case TypeIndex::Date:       return "Date";
        case TypeIndex::DateTime:   return "DateTime";
        case TypeIndex::DateTime64: return "DateTime64";
        case TypeIndex::String:     return TypeName<String>::get();
        case TypeIndex::FixedString: return "FixedString";
        case TypeIndex::Enum8:      return "Enum8";
        case TypeIndex::Enum16:     return "Enum16";
        case TypeIndex::Decimal32:  return TypeName<Decimal32>::get();
        case TypeIndex::Decimal64:  return TypeName<Decimal64>::get();
        case TypeIndex::Decimal128: return TypeName<Decimal128>::get();
        case TypeIndex::Decimal256: return TypeName<Decimal256>::get();
        case TypeIndex::UUID:       return "UUID";
        case TypeIndex::Array:      return "Array";
        case TypeIndex::Tuple:      return "Tuple";
        case TypeIndex::Set:        return "Set";
        case TypeIndex::Interval:   return "Interval";
        case TypeIndex::Nullable:   return "Nullable";
        case TypeIndex::Function:   return "Function";
        case TypeIndex::AggregateFunction: return "AggregateFunction";
        case TypeIndex::LowCardinality: return "LowCardinality";
    }

    __builtin_unreachable();
}

}

/// Specialization of `std::hash` for the Decimal<T> types.
namespace std
{
    template <typename T>
    struct hash<DB::Decimal<T>> { size_t operator()(const DB::Decimal<T> & x) const { return hash<T>()(x.value); } };

    template <>
    struct hash<DB::Decimal128>
    {
        size_t operator()(const DB::Decimal128 & x) const
        {
            return std::hash<DB::Int64>()(x.value >> 64)
                ^ std::hash<DB::Int64>()(x.value & std::numeric_limits<DB::UInt64>::max());
        }
    };


    template <>
    struct hash<DB::Decimal256>
    {
        size_t operator()(const DB::Decimal256 & x) const
        {
            // temp solution
            static constexpr DB::UInt64 max_uint_mask = std::numeric_limits<DB::UInt64>::max();
            return std::hash<DB::Int64>()(static_cast<DB::Int64>(x.value >> 64 & max_uint_mask))
                ^ std::hash<DB::Int64>()(static_cast<DB::Int64>(x.value & max_uint_mask));
        }
    };
}
