#pragma once

#include "diff_types.h"
#include "diff_value.h"

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <functional>

namespace diff {

// 单条 MQ 消息的内存表示（序列化前，仅供内部使用）
struct PendingMessage {
    std::string              request_id;
    std::string              service;
    std::string              region;
    Group                    group;    // BASE 或 TEST
    std::string              key;
    std::vector<std::pair<std::string, DiffValue>> fields;
    Tags                     tags;
    int64_t                  timestamp_ms;
};

// MQ 写入接口（可注入 mock 用于测试）
class MQProducer {
public:
    virtual ~MQProducer() = default;
    // 返回 true 表示发送成功
    virtual bool Send(const std::string& topic, const std::string& payload) = 0;
};

// 异步写入器：内存队列 + 后台线程池批量 flush 到 MQ
//
// 线程池大小优先级：
//   1. DiffConfig::thread_pool_size > 0  → 直接使用
//   2. 环境变量 GEC_DIFF_THREAD_POOL_SIZE → 解析为整数
//   3. 默认值：1（单线程，保证顺序）
class AsyncWriter {
public:
    explicit AsyncWriter(const DiffConfig& config, std::unique_ptr<MQProducer> producer);
    ~AsyncWriter();

    // 将消息推入内存队列，非阻塞，< 0.1ms
    // 队列满时丢弃并递增 drop_count_
    bool Enqueue(PendingMessage msg);

    // 等待队列清空（测试用）
    void Flush();

    uint64_t DropCount()    const { return drop_count_.load(); }
    int      ThreadCount()  const { return static_cast<int>(workers_.size()); }

private:
    void WorkerLoop();
    std::string Serialize(const PendingMessage& msg) const;

    // 从 config 或环境变量解析线程数
    static int ResolveThreadPoolSize(const DiffConfig& config);

    const DiffConfig&              config_;
    std::unique_ptr<MQProducer>    producer_;

    std::queue<PendingMessage>     queue_;
    std::mutex                     mu_;
    std::condition_variable        cv_;
    std::atomic<bool>              stop_{false};
    std::vector<std::thread>       workers_;   // 线程池

    std::atomic<uint64_t>          drop_count_{0};

    static constexpr int           kBatchSize = 64;
};

} // namespace diff
