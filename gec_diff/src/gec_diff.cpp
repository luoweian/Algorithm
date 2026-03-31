#include "gec_diff.h"
#include "async_writer.h"

#include <chrono>
#include <cstdlib>
#include <mutex>
#include <random>
#include <string>

namespace diff {

// ---- Stub MQ producer（生产环境替换为真实 BMQ/Kafka SDK）----
class StubMQProducer : public MQProducer {
public:
    bool Send(const std::string& /*topic*/, const std::string& /*payload*/) override {
        return true;
    }
};

// =========================================================================
// 内部辅助
// =========================================================================

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
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

const char* GetEnv(const char* key, const char* fallback = "") {
    const char* v = std::getenv(key);
    return (v && v[0]) ? v : fallback;
}

} // namespace

// =========================================================================
// GecDiff::GetInstance() — 懒初始化，线程安全
// =========================================================================

GecDiff* GecDiff::GetInstance() {
    // 快速路径：未启用时直接返回 nullptr（无锁、无 once overhead）
    static bool checked = false;
    static GecDiff* instance = nullptr;
    if (checked) return instance;

    static std::once_flag flag;
    std::call_once(flag, [] {
        checked = true;
        const char* enabled = std::getenv("GEC_DIFF_ENABLED");
        if (!enabled || std::string(enabled) != "1") {
            instance = nullptr;
            return;
        }

        // 从环境变量读取配置
        WriterConfig cfg;
        cfg.service_name = GetEnv("GEC_DIFF_SERVICE_NAME", "unknown_service");
        cfg.queue_size   = [] {
            const char* v = std::getenv("GEC_DIFF_QUEUE_SIZE");
            if (v) { int n = std::atoi(v); if (n > 0) return n; }
            return 10000;
        }();
        cfg.thread_pool_size = [] {
            const char* v = std::getenv("GEC_DIFF_THREAD_POOL_SIZE");
            if (v) { int n = std::atoi(v); if (n > 0) return n; }
            return 2;
        }();

        auto* obj = new GecDiff();
        obj->service_name_ = cfg.service_name;
        obj->region_       = GetEnv("GEC_DIFF_REGION", "ROW");
        obj->sample_rate_  = [] {
            const char* v = std::getenv("GEC_DIFF_SAMPLE_RATE");
            if (v) { float f = std::atof(v); if (f >= 0.0f) return f; }
            return 1.0f;
        }();
        obj->writer_ = new AsyncWriter(std::move(cfg),
                                       std::make_unique<StubMQProducer>());
        instance = obj;
    });
    return instance;
}

// =========================================================================
// 公开 API
// =========================================================================

bool GecDiff::IsEnabled() {
    return GetInstance() != nullptr;
}

void GecDiff::WriteBase(const std::string& request_id,
                         const FieldList&   fields,
                         const Tags&        tags) {
    BatchInternal(Group::BASE, request_id, fields, tags);
}

void GecDiff::WriteTest(const std::string& request_id,
                         const FieldList&   fields,
                         const Tags&        tags) {
    BatchInternal(Group::TEST, request_id, fields, tags);
}

// =========================================================================
// 内部实现
// =========================================================================

void GecDiff::WriteInternal(Group              group,
                              const std::string& request_id,
                              const std::string& key,
                              const DiffValue&   value,
                              const Tags&        tags) {
    GecDiff* inst = GetInstance();
    if (!inst) return;  // 未启用，零开销退出
    if (!ShouldSample(inst->sample_rate_)) return;

    PendingMessage msg;
    msg.request_id   = request_id;
    msg.service      = inst->service_name_;
    msg.region       = inst->region_;
    msg.group        = group;
    msg.key          = key;
    msg.fields       = {{"", value}};
    msg.tags         = tags;
    msg.timestamp_ms = NowMs();
    inst->writer_->Enqueue(std::move(msg));
}

void GecDiff::BatchInternal(Group              group,
                              const std::string& request_id,
                              const FieldList&   fields,
                              const Tags&        tags) {
    GecDiff* inst = GetInstance();
    if (!inst || fields.empty()) return;
    if (!ShouldSample(inst->sample_rate_)) return;

    PendingMessage msg;
    msg.request_id   = request_id;
    msg.service      = inst->service_name_;
    msg.region       = inst->region_;
    msg.group        = group;
    msg.key          = "";
    msg.fields       = fields;
    msg.tags         = tags;
    msg.timestamp_ms = NowMs();
    inst->writer_->Enqueue(std::move(msg));
}

} // namespace diff
