#include "diff_client.h"
#include "async_writer.h"

#include <chrono>
#include <random>
#include <stdexcept>

namespace diff {

// ---- 默认 MQ Producer（生产替换为真实 Kafka / RocketMQ SDK）----
class StubMQProducer : public MQProducer {
public:
    bool Send(const std::string& /*topic*/, const std::string& /*payload*/) override {
        return true;
    }
};

// =========================================================================
// DiffScope 实现
// =========================================================================

DiffScope::DiffScope(DiffClient& client,
                     std::string  request_id,
                     Group        group,
                     Tags         base_tags)
    : client_(client)
    , request_id_(std::move(request_id))
    , group_(group)
    , base_tags_(std::move(base_tags)) {}

Tags DiffScope::MergeTags(const Tags& extra_tags) const {
    if (extra_tags.empty()) return base_tags_;
    Tags merged = base_tags_;
    for (const auto& [k, v] : extra_tags) merged[k] = v;
    return merged;
}

void DiffScope::WriteImpl(const std::string& key,
                          const DiffValue&   value,
                          const Tags&        extra_tags) {
    client_.EnqueueSingle(request_id_, key, group_, value,
                          MergeTags(extra_tags));
}

DiffScope& DiffScope::WriteBatch(const std::string& key,
                                 const FieldList&   fields,
                                 const Tags&        extra_tags) {
    client_.EnqueueBatch(request_id_, key, group_, fields,
                         MergeTags(extra_tags));
    return *this;
}

// =========================================================================
// DiffClient 实现
// =========================================================================

DiffClient* DiffClient::instance_ = nullptr;

bool DiffClient::Init(DiffConfig config) {
    if (instance_) return false;
    instance_ = new DiffClient();
    return instance_->InitInternal(std::move(config));
}

void DiffClient::Shutdown() {
    delete instance_;
    instance_ = nullptr;
}

DiffClient& DiffClient::Get() {
    if (!instance_) throw std::runtime_error("DiffClient::Get() before Init()");
    return *instance_;
}

bool DiffClient::InitInternal(DiffConfig config) {
    config_ = std::move(config);
    writer_ = std::make_unique<AsyncWriter>(config_, std::make_unique<StubMQProducer>());
    return true;
}

DiffClient::~DiffClient() = default;

DiffScope DiffClient::NewScope(const std::string& request_id,
                                Group              group,
                                const Tags&        base_tags) {
    return DiffScope(*this, request_id, group, base_tags);
}

// ---- 内部辅助 ----
namespace {
bool ShouldSample(float rate) {
    if (rate >= 1.0f) return true;
    if (rate <= 0.0f) return false;
    static thread_local std::mt19937 rng(std::random_device{}());
    static thread_local std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(rng) < rate;
}
int64_t NowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}
}

void DiffClient::EnqueueSingle(const std::string& request_id,
                                const std::string& key,
                                Group              group,
                                const DiffValue&   value,
                                const Tags&        tags) {
    if (!ShouldSample(config_.sample_rate)) return;
    PendingMessage msg;
    msg.request_id   = request_id;
    msg.service      = config_.service_name;
    msg.region       = config_.region;
    msg.group        = group;
    msg.key          = key;
    msg.fields       = {{"", value}};
    msg.tags         = tags;
    msg.timestamp_ms = NowMs();
    writer_->Enqueue(std::move(msg));
}

void DiffClient::EnqueueBatch(const std::string& request_id,
                               const std::string& key,
                               Group              group,
                               const FieldList&   fields,
                               const Tags&        tags) {
    if (!ShouldSample(config_.sample_rate) || fields.empty()) return;
    PendingMessage msg;
    msg.request_id   = request_id;
    msg.service      = config_.service_name;
    msg.region       = config_.region;
    msg.group        = group;
    msg.key          = key;
    msg.fields       = fields;
    msg.tags         = tags;
    msg.timestamp_ms = NowMs();
    writer_->Enqueue(std::move(msg));
}

} // namespace diff
