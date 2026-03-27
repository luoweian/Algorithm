#include "async_writer.h"

#include <chrono>
#include <sstream>
#include <cstring>

// 注意：实际项目中替换为 protobuf 序列化
// 此处用简单的二进制格式演示，保证可编译运行
// 格式：[field_count:4][name_len:2][name][type:1][value_len:4][value]...

namespace diff {

namespace {

void WriteUint16(std::string& out, uint16_t v) {
    out.push_back(static_cast<char>(v & 0xFF));
    out.push_back(static_cast<char>((v >> 8) & 0xFF));
}

void WriteUint32(std::string& out, uint32_t v) {
    out.append(reinterpret_cast<const char*>(&v), 4);
}

void WriteString(std::string& out, const std::string& s) {
    WriteUint32(out, static_cast<uint32_t>(s.size()));
    out.append(s);
}

void WriteInt64(std::string& out, int64_t v) {
    out.append(reinterpret_cast<const char*>(&v), 8);
}

} // namespace

AsyncWriter::AsyncWriter(const DiffConfig& config, std::unique_ptr<MQProducer> producer)
    : config_(config), producer_(std::move(producer)) {
    worker_ = std::thread(&AsyncWriter::WorkerLoop, this);
}

AsyncWriter::~AsyncWriter() {
    stop_.store(true);
    cv_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }
}

bool AsyncWriter::Enqueue(PendingMessage msg) {
    std::unique_lock<std::mutex> lock(mu_);
    if (static_cast<int>(queue_.size()) >= config_.async_queue_size) {
        drop_count_.fetch_add(1);
        return false;
    }
    queue_.push(std::move(msg));
    lock.unlock();
    cv_.notify_one();
    return true;
}

void AsyncWriter::Flush() {
    // 等待队列为空
    while (true) {
        {
            std::unique_lock<std::mutex> lock(mu_);
            if (queue_.empty()) return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void AsyncWriter::WorkerLoop() {
    std::vector<PendingMessage> batch;
    batch.reserve(kBatchSize);

    while (!stop_.load() || !queue_.empty()) {
        {
            std::unique_lock<std::mutex> lock(mu_);
            cv_.wait_for(lock, std::chrono::milliseconds(10), [this] {
                return !queue_.empty() || stop_.load();
            });

            int count = 0;
            while (!queue_.empty() && count < kBatchSize) {
                batch.push_back(std::move(queue_.front()));
                queue_.pop();
                ++count;
            }
        }

        for (auto& msg : batch) {
            // OLD/NEW 分 topic 发送，Flink 从两个 topic 做 Stream Join
            std::string topic = config_.service_name + ".diff."
                                + (msg.side == Side::OLD ? "old" : "new");
            std::string payload = Serialize(msg);
            if (!producer_->Send(topic, payload)) {
                drop_count_.fetch_add(1);
            }
        }
        batch.clear();
    }
}

std::string AsyncWriter::Serialize(const PendingMessage& msg) const {
    // 简易二进制序列化（生产替换为 protobuf）
    // Header: request_id | service | region | side(1) | key | tags_count | tags | timestamp | fields_count
    std::string out;
    out.reserve(256);

    WriteString(out, msg.request_id);
    WriteString(out, msg.service);
    WriteString(out, msg.region);
    out.push_back(static_cast<char>(msg.side == Side::OLD ? 1 : 2));
    WriteString(out, msg.key);

    // tags
    WriteUint32(out, static_cast<uint32_t>(msg.tags.size()));
    for (const auto& [k, v] : msg.tags) {
        WriteString(out, k);
        WriteString(out, v);
    }

    WriteInt64(out, msg.timestamp_ms);

    // fields
    WriteUint32(out, static_cast<uint32_t>(msg.fields.size()));
    for (const auto& [name, val] : msg.fields) {
        WriteString(out, name);
        out.push_back(static_cast<char>(static_cast<int>(val.type)));
        std::string serialized = val.Serialize();
        WriteString(out, serialized);
    }

    return out;
}

} // namespace diff
