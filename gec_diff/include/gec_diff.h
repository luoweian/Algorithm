#pragma once

/**
 * GecDiff — 轻量级 A/B Diff 埋点库
 *
 * 环境变量：
 *   GEC_DIFF_ENABLED=1            启用（默认关闭）
 *   GEC_DIFF_SERVICE_NAME=svc     服务名
 *   GEC_DIFF_REGION=ROW           数据域（ROW/EU/TTP，默认 ROW）
 *   GEC_DIFF_SAMPLE_RATE=1.0      采样率 0.0~1.0
 *   GEC_DIFF_THREAD_POOL_SIZE=4   写线程数（默认 2）
 *   GEC_DIFF_QUEUE_SIZE=10000     队列深度（默认 10000）
 *
 * 用法：
 *   // 单值
 *   diff::Write(Group::BASE, request_id, "ctr_score", 0.12, tags);
 *   diff::Write(Group::TEST, request_id, "ctr_score", 0.13, tags);
 *
 *   // 批量（已构造好 DiffValue 列表）
 *   diff::Write(Group::BASE, request_id, {{"ctr", DiffValue(0.12)}, {"uid", DiffValue(42)}}, tags);
 */

#include "diff_types.h"
#include "diff_value.h"

#include <string>
#include <vector>

namespace diff {

using FieldList = std::vector<std::pair<std::string, DiffValue>>;

class AsyncWriter;

// ---- MQ 写入层（接收已转换好的 DiffValue，不做类型推导）----
class GecDiff {
public:
    static void Write(Group              group,
                      const std::string& request_id,
                      const std::string& key,
                      DiffValue          value,
                      const Tags&        tags = {});

    static void Write(Group              group,
                      const std::string& request_id,
                      const FieldList&   fields,
                      const Tags&        tags = {});

    static bool IsEnabled();

private:
    GecDiff() = default;
    static GecDiff* GetInstance();

    AsyncWriter* writer_       = nullptr;
    std::string  service_name_;
    std::string  region_;
    float        sample_rate_  = 1.0f;
};

// ---- 类型转换层（模板，T → DiffValue，与 MQ 写入无关）----

template <typename T>
inline void Write(Group              group,
                  const std::string& request_id,
                  const std::string& key,
                  T&&                value,
                  const Tags&        tags = {}) {
    GecDiff::Write(group, request_id, key,
                   DiffValue(std::forward<T>(value)), tags);
}

} // namespace diff
