#pragma once

#include <string>
#include <cstdint>
#include <stdexcept>

namespace diff {

enum class ValueType {
    INT32,
    INT64,
    UINT32,
    UINT64,
    FLOAT32,
    FLOAT64,
    BOOL,
    STRING,
    NULL_TYPE,
    BYTES,
    JSON,       // 递归结构，内部为 JSON 序列化字符串
};

// 统一值表示，内部用 union 存储数值类型，str 存字符串/字节/JSON
struct DiffValue {
    ValueType type = ValueType::NULL_TYPE;

    union {
        int32_t  i32;
        int64_t  i64;
        uint32_t u32;
        uint64_t u64;
        float    f32;
        double   f64;
        bool     b;
    };
    std::string str; // STRING / BYTES / JSON 共用

    // ---- 工厂方法 ----
    static DiffValue Int32(int32_t v)       { DiffValue d; d.type = ValueType::INT32;   d.i32 = v; return d; }
    static DiffValue Int64(int64_t v)       { DiffValue d; d.type = ValueType::INT64;   d.i64 = v; return d; }
    static DiffValue UInt32(uint32_t v)     { DiffValue d; d.type = ValueType::UINT32;  d.u32 = v; return d; }
    static DiffValue UInt64(uint64_t v)     { DiffValue d; d.type = ValueType::UINT64;  d.u64 = v; return d; }
    static DiffValue Float32(float v)       { DiffValue d; d.type = ValueType::FLOAT32; d.f32 = v; return d; }
    static DiffValue Float64(double v)      { DiffValue d; d.type = ValueType::FLOAT64; d.f64 = v; return d; }
    static DiffValue Bool(bool v)           { DiffValue d; d.type = ValueType::BOOL;    d.b   = v; return d; }
    static DiffValue String(std::string v)  { DiffValue d; d.type = ValueType::STRING;  d.str = std::move(v); return d; }
    static DiffValue Bytes(std::string v)   { DiffValue d; d.type = ValueType::BYTES;   d.str = std::move(v); return d; }
    static DiffValue Json(std::string v)    { DiffValue d; d.type = ValueType::JSON;    d.str = std::move(v); return d; }
    static DiffValue Null()                 { DiffValue d; d.type = ValueType::NULL_TYPE; return d; }

    DiffValue() : i64(0) {}

    const char* TypeName() const {
        switch (type) {
            case ValueType::INT32:     return "int32";
            case ValueType::INT64:     return "int64";
            case ValueType::UINT32:    return "uint32";
            case ValueType::UINT64:    return "uint64";
            case ValueType::FLOAT32:   return "float32";
            case ValueType::FLOAT64:   return "float64";
            case ValueType::BOOL:      return "bool";
            case ValueType::STRING:    return "string";
            case ValueType::NULL_TYPE: return "null";
            case ValueType::BYTES:     return "bytes";
            case ValueType::JSON:      return "json";
        }
        return "unknown";
    }

    std::string DebugString() const;

    // 序列化为 bytes（写入 proto DiffField.value）
    std::string Serialize() const;

    // 从 bytes 反序列化
    static DiffValue Deserialize(ValueType type, const std::string& bytes);
};

} // namespace diff
