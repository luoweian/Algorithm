#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace diff {

// 数据侧
enum class Side {
    OLD,
    NEW,
};

// Diff 比较模式
enum class DiffMode {
    STRICT,   // 类型不同视为 type_change
    LOOSE,    // 数值类型等价则视为相同
};

// 数组 Diff 模式
enum class ArrayDiffMode {
    ORDERED,    // 按索引对齐（默认）
    UNORDERED,  // 按内容匹配，适合集合语义
};

// Tag map
using Tags = std::unordered_map<std::string, std::string>;

// Diff 配置
struct DiffConfig {
    std::string mq_endpoint;
    std::string service_name;
    std::string region;                    // ROW | EU | TTP

    float       sample_rate       = 1.0f; // 写入 MQ 的全局采样率 [0.0, 1.0]
    DiffMode    diff_mode         = DiffMode::STRICT;

    double      float_abs_epsilon = 1e-6; // 浮点绝对误差
    double      float_rel_epsilon = 1e-4; // 浮点相对误差

    std::vector<std::string> ignore_paths; // 忽略的 JSON 路径，如 /timestamp

    int         async_queue_size  = 10000; // 异步写入队列大小
};

// Diff 操作类型（Flink 输出使用）
enum class DiffOp {
    EQUAL,
    VALUE_CHANGE,
    TYPE_CHANGE,
    ADDED,
    REMOVED,
};

inline const char* DiffOpToString(DiffOp op) {
    switch (op) {
        case DiffOp::EQUAL:        return "equal";
        case DiffOp::VALUE_CHANGE: return "value_change";
        case DiffOp::TYPE_CHANGE:  return "type_change";
        case DiffOp::ADDED:        return "added";
        case DiffOp::REMOVED:      return "removed";
    }
    return "unknown";
}

} // namespace diff
