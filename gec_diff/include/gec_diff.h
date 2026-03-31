#pragma once

/**
 * GecDiff — 轻量级 A/B Diff 埋点库
 *
 * 通过环境变量控制行为，用户代码零侵入：
 *
 *   GEC_DIFF_ENABLED=1            启用（默认关闭）
 *   GEC_DIFF_SERVICE_NAME=svc     服务名（必填，启用时）
 *   GEC_DIFF_MQ_ENDPOINT=url      MQ endpoint（暂用 stub，预留）
 *   GEC_DIFF_REGION=ROW           数据域（ROW/EU/TTP，默认 ROW）
 *   GEC_DIFF_SAMPLE_RATE=1.0      采样率 0.0~1.0（默认 1.0）
 *   GEC_DIFF_THREAD_POOL_SIZE=4   写线程数（默认 2）
 *   GEC_DIFF_QUEUE_SIZE=10000     内存队列深度（默认 10000）
 *
 * 用法示例：
 *   // 单值写入
 *   GecDiff::WriteBase(request_id, "ctr_score", 0.12, tags);
 *   GecDiff::WriteTest(request_id, "ctr_score", 0.13, tags);
 *
 *   // 批量写入
 *   GecDiff::WriteBase(request_id, {{"ctr", 0.12}, {"uid", 12345}}, tags);
 */

#include "diff_types.h"
#include "diff_value.h"

#include <string>
#include <vector>

namespace diff {

using FieldList = std::vector<std::pair<std::string, DiffValue>>;

class AsyncWriter;

class GecDiff {
public:
    // ---- BASE（对照组 / 旧逻辑）----

    // 单值写入：自动推导类型（int/float/bool/string/...）
    template <typename T>
    static void WriteBase(const std::string& request_id,
                          const std::string& key,
                          T&&                value,
                          const Tags&        tags = {}) {
        WriteInternal(Group::BASE, request_id, key,
                      DiffValue(std::forward<T>(value)), tags);
    }

    // 批量写入
    static void WriteBase(const std::string& request_id,
                          const FieldList&   fields,
                          const Tags&        tags = {});

    // ---- TEST（实验组 / 新逻辑）----

    template <typename T>
    static void WriteTest(const std::string& request_id,
                          const std::string& key,
                          T&&                value,
                          const Tags&        tags = {}) {
        WriteInternal(Group::TEST, request_id, key,
                      DiffValue(std::forward<T>(value)), tags);
    }

    static void WriteTest(const std::string& request_id,
                          const FieldList&   fields,
                          const Tags&        tags = {});

    // 判断当前是否启用（主要供测试使用）
    static bool IsEnabled();

private:
    GecDiff() = default;

    // 懒初始化单例：GEC_DIFF_ENABLED != "1" 时返回 nullptr
    static GecDiff* GetInstance();

    static void WriteInternal(Group              group,
                               const std::string& request_id,
                               const std::string& key,
                               const DiffValue&   value,
                               const Tags&        tags);

    static void BatchInternal(Group              group,
                               const std::string& request_id,
                               const FieldList&   fields,
                               const Tags&        tags);

    AsyncWriter* writer_ = nullptr;  // 由构造函数创建，与 GecDiff 同生命周期
    std::string  service_name_;
    std::string  region_;
    float        sample_rate_ = 1.0f;
};

} // namespace diff
