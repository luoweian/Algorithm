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

    // 写入单个值
    // request_id: 用于 Flink Join 的关联键
    // key:        数据标识，同一请求的 OLD/NEW 必须使用相同 key
    // side:       Side::OLD 或 Side::NEW
    // value:      DiffValue，支持所有基础类型和 JSON
    // tags:       业务标签，用于 Metrics 聚合维度
    void Write(
        const std::string& request_id,
        const std::string& key,
        Side               side,
        const DiffValue&   value,
        const Tags&        tags = {}
    );

    // 批量写入（减少 MQ 消息数，推荐用于多字段场景）
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

    DiffConfig                   config_;
    std::unique_ptr<AsyncWriter> writer_;

    static DiffClient* instance_;
};

} // namespace diff
