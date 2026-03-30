#pragma once

#include "diff_types.h"
#include "diff_value.h"

#include <string>
#include <vector>
#include <memory>
#include <initializer_list>

namespace diff {

// 批量写入的字段列表
using FieldList = std::vector<std::pair<std::string, DiffValue>>;

class AsyncWriter; // 前向声明

// DiffClient：单例，全局初始化一次，线程安全
class DiffClient {
public:
    // 初始化（进程启动时调用一次）
    static bool Init(DiffConfig config);

    // 销毁（进程退出前调用）
    static void Shutdown();

    // 获取单例
    static DiffClient& Get();

    // 写入单个值（模板：自动推断类型，无需手动指定 DiffValue::Float64 等）
    // 支持所有原生类型：int32_t, int64_t, uint32_t, uint64_t,
    //                   float, double, bool, std::string, const char*
    // JSON 类型仍需显式：DiffValue::Json(str)
    //
    // 示例：
    //   Write(req_id, "ctr_score", Side::OLD, 0.12, tags);
    //   Write(req_id, "user_id",   Side::OLD, 12345, tags);
    //   Write(req_id, "is_vip",    Side::OLD, true, tags);
    //   Write(req_id, "features",  Side::OLD, DiffValue::Json(json_str), tags);
    template<typename T>
    void Write(
        const std::string& request_id,
        const std::string& key,
        Side               side,
        T&&                value,
        const Tags&        tags = {}
    ) {
        WriteImpl(request_id, key, side, DiffValue(std::forward<T>(value)), tags);
    }

    // 批量写入（减少 MQ 消息数，推荐用于多字段场景）
    // FieldList 元素靠隐式构造自动匹配类型，无需手动包装：
    //
    // 示例：
    //   WriteBatch(req_id, "model", Side::OLD, {
    //       {"ctr_score",  0.12},          // double  → FLOAT64
    //       {"cvr_score",  0.05f},         // float   → FLOAT32
    //       {"user_id",    12345},         // int     → INT32
    //       {"is_vip",     true},          // bool    → BOOL
    //       {"user_name",  "alice"},       // string  → STRING
    //       {"feat_json",  DiffValue::Json(json_str)},  // JSON 需显式
    //   }, tags);
    void WriteBatch(
        const std::string& request_id,
        const std::string& key,
        Side               side,
        const FieldList&   fields,
        const Tags&        tags = {}
    );

    // 当前配置（只读）
    const DiffConfig& Config() const { return config_; }

private:
    DiffClient() = default;
    ~DiffClient();

    bool InitInternal(DiffConfig config);

    // Write 模板的实际实现（非模板，避免头文件膨胀）
    void WriteImpl(
        const std::string& request_id,
        const std::string& key,
        Side               side,
        const DiffValue&   value,
        const Tags&        tags
    );

    DiffConfig                   config_;
    std::unique_ptr<AsyncWriter> writer_;

    static DiffClient* instance_;
};

} // namespace diff
