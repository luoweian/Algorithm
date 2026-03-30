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

// 单条 MQ 消息的内存表示（序列化前）
struct PendingMessage {
    std::string              request_id;
    std::string              service;
    std::string              region;
    Side                     side;
    std::string              key;
    std::vector<std::pair<std::string, DiffValue>> fields; // name -> value
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

// 异步写入器：内存队列 + 后台线程批量 flush 到 MQ
class AsyncWriter {
public:
    explicit AsyncWriter(const DiffConfig& config, std::unique_ptr<MQProducer> producer);
    ~AsyncWriter();

    // 将消息推入内存队列，非阻塞，< 0.1ms
    // 队列满时丢弃并递增 drop_count_
    bool Enqueue(PendingMessage msg);

    // 等待队列清空（测试用）
    void Flush();

    uint64_t DropCount() const { return drop_count_.load(); }

private:
    void WorkerLoop();

    // 将 PendingMessage 序列化为 proto bytes
    std::string Serialize(const PendingMessage& msg) const;

    const DiffConfig&              config_;
    std::unique_ptr<MQProducer>    producer_;

    std::queue<PendingMessage>     queue_;
    std::mutex                     mu_;
    std::condition_variable        cv_;
    std::atomic<bool>              stop_{false};
    std::thread                    worker_;

    std::atomic<uint64_t>          drop_count_{0};

    static constexpr int           kBatchSize = 64; // 每次最多批量发送条数
};

} // namespace diff
