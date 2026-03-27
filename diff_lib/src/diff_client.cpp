#include "diff_client.h"
#include "async_writer.h"

#include <chrono>
#include <random>
#include <stdexcept>

namespace diff {

// ---- 默认 MQ Producer（生产替换为真实实现） ----
class StubMQProducer : public MQProducer {
public:
    bool Send(const std::string& /*topic*/, const std::string& /*payload*/) override {
        // TODO: 替换为真实 MQ SDK（Kafka / RocketMQ 等）调用
        return true;
    }
};

// ---- 静态成员 ----
DiffClient* DiffClient::instance_ = nullptr;

bool DiffClient::Init(DiffConfig config) {
    if (instance_) return false; // 已初始化
    instance_ = new DiffClient();
    return instance_->InitInternal(std::move(config));
}

void DiffClient::Shutdown() {
    delete instance_;
    instance_ = nullptr;
}

DiffClient& DiffClient::Get() {
    if (!instance_) {
        throw std::runtime_error("DiffClient::Get() called before Init()");
    }
    return *instance_;
}

bool DiffClient::InitInternal(DiffConfig config) {
    config_ = std::move(config);
    writer_ = std::make_unique<AsyncWriter>(config_, std::make_unique<StubMQProducer>());
    return true;
}

DiffClient::~DiffClient() = default;

// ---- 采样判断 ----
namespace {
bool ShouldSample(float sample_rate) {
    if (sample_rate >= 1.0f) return true;
    if (sample_rate <= 0.0f) return false;
    static thread_local std::mt19937 rng(std::random_device{}());
    static thread_local std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(rng) < sample_rate;
}

int64_t NowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}
} // namespace

// ---- Write ----
void DiffClient::Write(
    const std::string& request_id,
    const std::string& key,
    Side               side,
    const DiffValue&   value,
    const Tags&        tags)
{
    if (!ShouldSample(config_.sample_rate)) return;

    PendingMessage msg;
    msg.request_id   = request_id;
    msg.service      = config_.service_name;
    msg.region       = config_.region;
    msg.side         = side;
    msg.key          = key;
    msg.fields       = {{"", value}};
    msg.tags         = tags;
    msg.timestamp_ms = NowMs();

    writer_->Enqueue(std::move(msg));
}

// ---- WriteBatch ----
void DiffClient::WriteBatch(
    const std::string& request_id,
    const std::string& key,
    Side               side,
    const FieldList&   fields,
    const Tags&        tags)
{
    if (!ShouldSample(config_.sample_rate)) return;
    if (fields.empty()) return;

    PendingMessage msg;
    msg.request_id   = request_id;
    msg.service      = config_.service_name;
    msg.region       = config_.region;
    msg.side         = side;
    msg.key          = key;
    msg.fields       = fields;
    msg.tags         = tags;
    msg.timestamp_ms = NowMs();

    writer_->Enqueue(std::move(msg));
}

} // namespace diff
