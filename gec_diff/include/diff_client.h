#pragma once

#include "diff_types.h"
#include "diff_value.h"

#include <string>
#include <vector>
#include <memory>

namespace diff {

// 批量写入的字段列表（隐式构造，无需手动 DiffValue::Float64 等）
using FieldList = std::vector<std::pair<std::string, DiffValue>>;

class DiffClient;
class AsyncWriter;

// ---------------------------------------------------------------------------
// DiffScope：单次请求的写入上下文
//
// 通过 DiffClient::Get().NewScope(request_id, side, tags) 创建。
// 用户只需传 key + value，request_id / side / tags 均在 Scope 内部管理。
//
// 典型用法：
//   auto old_scope = DiffClient::Get().NewScope(request_id, Side::OLD,
//                       {{"scene", "rank"}, {"layer", "feature"}});
//   old_scope.Write("ctr_score", 0.12);
//   old_scope.Write("uid",       12345);
//   old_scope.Write("is_vip",    true);
//   old_scope.Write("feat_json", DiffValue::Json(json_str)); // JSON 需显式
//
//   // 多字段一次性写入（单条 MQ 消息，更高效）
//   old_scope.WriteBatch("feature_map", {
//       {"ctr",  0.12},
//       {"cvr",  0.05},
//       {"uid",  12345},
//   });
// ---------------------------------------------------------------------------
class DiffScope {
public:
    // 写入单个 key-value（类型自动推断）
    // extra_tags 会与 Scope 的 base_tags 合并（extra_tags 优先）
    template<typename T>
    DiffScope& Write(const std::string& key, T&& value,
                     const Tags& extra_tags = {}) {
        WriteImpl(key, DiffValue(std::forward<T>(value)), extra_tags);
        return *this;
    }

    // 批量写入多个字段（单条 MQ 消息，推荐用于特征 Map 等多字段场景）
    DiffScope& WriteBatch(const std::string& key, const FieldList& fields,
                          const Tags& extra_tags = {});

private:
    friend class DiffClient;

    DiffScope(DiffClient& client,
              std::string  request_id,
              Side         side,
              Tags         base_tags);

    void WriteImpl(const std::string& key,
                   const DiffValue&   value,
                   const Tags&        extra_tags);

    Tags MergeTags(const Tags& extra_tags) const;

    DiffClient&  client_;
    std::string  request_id_;
    Side         side_;
    Tags         base_tags_;
};

// ---------------------------------------------------------------------------
// DiffClient：全局单例，进程启动时 Init 一次
// ---------------------------------------------------------------------------
class DiffClient {
public:
    static bool    Init(DiffConfig config);
    static void    Shutdown();
    static DiffClient& Get();

    // 创建写入 Scope（推荐入口）
    // request_id : 用于 Flink Join 的关联键
    // side       : Side::OLD（旧逻辑） 或 Side::NEW（新逻辑）
    // base_tags  : 本次请求的公共标签（scene / layer / exp_id 等）
    DiffScope NewScope(const std::string& request_id,
                       Side               side,
                       const Tags&        base_tags = {});

    const DiffConfig& Config() const { return config_; }

private:
    friend class DiffScope;

    DiffClient() = default;
    ~DiffClient();
    bool InitInternal(DiffConfig config);

    // DiffScope 内部调用
    void EnqueueSingle(const std::string& request_id,
                       const std::string& key,
                       Side               side,
                       const DiffValue&   value,
                       const Tags&        tags);

    void EnqueueBatch(const std::string& request_id,
                      const std::string& key,
                      Side               side,
                      const FieldList&   fields,
                      const Tags&        tags);

    DiffConfig                   config_;
    std::unique_ptr<AsyncWriter> writer_;

    static DiffClient* instance_;
};

} // namespace diff
