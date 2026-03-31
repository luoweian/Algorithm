#include "async_writer.h"

#include <chrono>
#include <cstdlib>
#include <stdexcept>

namespace diff {

namespace {

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

// ---- 线程数解析 ----
int AsyncWriter::ResolveThreadPoolSize(const DiffConfig& config) {
    // 优先级 1：config 显式指定
    if (config.thread_pool_size > 0) return config.thread_pool_size;

    // 优先级 2：环境变量 GEC_DIFF_THREAD_POOL_SIZE
    const char* env = std::getenv("GEC_DIFF_THREAD_POOL_SIZE");
    if (env) {
        int n = std::atoi(env);
        if (n > 0) return n;
    }

    // 优先级 3：默认单线程
    return 1;
}

// ---- 构造 / 析构 ----
AsyncWriter::AsyncWriter(const DiffConfig& config, std::unique_ptr<MQProducer> producer)
    : config_(config), producer_(std::move(producer)) {
    int n = ResolveThreadPoolSize(config);
    workers_.reserve(n);
    for (int i = 0; i < n; ++i) {
        workers_.emplace_back(&AsyncWriter::WorkerLoop, this);
    }
}

AsyncWriter::~AsyncWriter() {
    stop_.store(true);
    cv_.notify_all();
    for (auto& w : workers_) {
        if (w.joinable()) w.join();
    }
}

// ---- Enqueue ----
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

// ---- Flush ----
void AsyncWriter::Flush() {
    while (true) {
        {
            std::unique_lock<std::mutex> lock(mu_);
            if (queue_.empty()) return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// ---- WorkerLoop（每个线程运行） ----
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
            // BASE/TEST 分 topic 发送，Flink 从两个 topic 做 Stream Join
            std::string topic = config_.service_name + ".diff."
                + (msg.group == Group::BASE ? "base" : "test");
            if (!producer_->Send(topic, Serialize(msg))) {
                drop_count_.fetch_add(1);
            }
        }
        batch.clear();
    }
}

// ---- Serialize ----
std::string AsyncWriter::Serialize(const PendingMessage& msg) const {
    std::string out;
    out.reserve(256);

    WriteString(out, msg.request_id);
    WriteString(out, msg.service);
    WriteString(out, msg.region);
    out.push_back(static_cast<char>(msg.group == Group::BASE ? 1 : 2));
    WriteString(out, msg.key);

    WriteUint32(out, static_cast<uint32_t>(msg.tags.size()));
    for (const auto& [k, v] : msg.tags) {
        WriteString(out, k);
        WriteString(out, v);
    }

    WriteInt64(out, msg.timestamp_ms);

    WriteUint32(out, static_cast<uint32_t>(msg.fields.size()));
    for (const auto& [name, val] : msg.fields) {
        WriteString(out, name);
        out.push_back(static_cast<char>(static_cast<int>(val.type)));
        WriteString(out, val.Serialize());
    }

    return out;
}

} // namespace diff
