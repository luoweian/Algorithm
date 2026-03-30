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

    // ---- 隐式构造（原生类型自动转换，Write/WriteBatch 无需手动指定类型）----
    DiffValue()                        : type(ValueType::NULL_TYPE), i64(0) {}
    DiffValue(int32_t v)               : type(ValueType::INT32),     i32(v) {}
    DiffValue(int64_t v)               : type(ValueType::INT64),     i64(v) {}
    DiffValue(uint32_t v)              : type(ValueType::UINT32),    u32(v) {}
    DiffValue(uint64_t v)              : type(ValueType::UINT64),    u64(v) {}
    DiffValue(float v)                 : type(ValueType::FLOAT32),   f32(v) {}
    DiffValue(double v)                : type(ValueType::FLOAT64),   f64(v) {}
    DiffValue(bool v)                  : type(ValueType::BOOL),      b(v)   {}
    DiffValue(std::string v)           : type(ValueType::STRING),    i64(0), str(std::move(v)) {}
    DiffValue(const char* v)           : type(ValueType::STRING),    i64(0), str(v)            {}

    // ---- JSON / Bytes 仍需显式标记类型 ----
    static DiffValue Json(std::string v)  { DiffValue d; d.type = ValueType::JSON;  d.str = std::move(v); return d; }
    static DiffValue Bytes(std::string v) { DiffValue d; d.type = ValueType::BYTES; d.str = std::move(v); return d; }

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
