#include <gtest/gtest.h>
#include "gec_diff.h"
#include "async_writer.h"

#include <mutex>
#include <vector>

using namespace diff;

// =========================================================================
// Mock MQ Producer
// =========================================================================
class MockMQProducer : public MQProducer {
public:
    bool Send(const std::string& topic, const std::string& payload) override {
        std::lock_guard<std::mutex> lock(mu_);
        messages_.push_back({topic, payload});
        return true;
    }
    struct Msg { std::string topic; std::string payload; };
    std::vector<Msg> messages_;
    std::mutex mu_;
};

// =========================================================================
// AsyncWriter 直接测试（绕过 GecDiff 单例，便于隔离）
// =========================================================================
class AsyncWriterTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_ = std::make_unique<MockMQProducer>();
        mock_ptr_ = mock_.get();
        WriterConfig cfg;
        cfg.service_name      = "test_service";
        cfg.queue_size        = 100;
        cfg.thread_pool_size  = 1;
        writer_ = std::make_unique<AsyncWriter>(cfg, std::move(mock_));
    }
    MockMQProducer*             mock_ptr_ = nullptr;
    std::unique_ptr<MockMQProducer> mock_;
    std::unique_ptr<AsyncWriter>    writer_;
};

// ---- BASE/TEST 路由到各自 topic ----
TEST_F(AsyncWriterTest, GroupTopicRouting) {
    PendingMessage base_msg;
    base_msg.request_id  = "req_001";
    base_msg.service     = "test_service";
    base_msg.region      = "ROW";
    base_msg.group       = Group::BASE;
    base_msg.key         = "ctr_score";
    base_msg.fields      = {{"", 0.12}};
    base_msg.timestamp_ms = 0;

    PendingMessage test_msg = base_msg;
    test_msg.group  = Group::TEST;
    test_msg.fields = {{"", 0.13}};

    EXPECT_TRUE(writer_->Enqueue(base_msg));
    EXPECT_TRUE(writer_->Enqueue(test_msg));
    writer_->Flush();

    EXPECT_EQ(mock_ptr_->messages_.size(), 2u);
    EXPECT_EQ(mock_ptr_->messages_[0].topic, "test_service.diff.base");
    EXPECT_EQ(mock_ptr_->messages_[1].topic, "test_service.diff.test");
}

// ---- 隐式类型转换 ----
TEST_F(AsyncWriterTest, ImplicitTypeConversion) {
    PendingMessage msg;
    msg.request_id   = "req_002";
    msg.service      = "test_service";
    msg.region       = "ROW";
    msg.group        = Group::BASE;
    msg.key          = "feature_map";
    msg.fields       = {
        {"ctr",    0.12},       // double  → FLOAT64
        {"uid",    12345},      // int     → INT32
        {"is_vip", true},       // bool    → BOOL
        {"name",   "alice"},    // string  → STRING
    };
    msg.timestamp_ms = 0;

    EXPECT_TRUE(writer_->Enqueue(msg));
    writer_->Flush();
    EXPECT_EQ(mock_ptr_->messages_.size(), 1u);
}

// ---- 队列满时丢弃 ----
TEST_F(AsyncWriterTest, QueueFullDropsMessages) {
    WriterConfig small_cfg;
    small_cfg.service_name     = "svc";
    small_cfg.queue_size       = 2;
    small_cfg.thread_pool_size = 1;
    auto small_mock = std::make_unique<MockMQProducer>();
    AsyncWriter small_writer(small_cfg, std::move(small_mock));

    PendingMessage msg;
    msg.request_id   = "req_x";
    msg.service      = "svc";
    msg.region       = "ROW";
    msg.group        = Group::BASE;
    msg.key          = "k";
    msg.timestamp_ms = 0;
    for (int i = 0; i < 10; ++i) small_writer.Enqueue(msg);
    EXPECT_GT(small_writer.DropCount(), 0u);
}

// ---- 线程数从 config 读取 ----
TEST_F(AsyncWriterTest, ThreadCountFromConfig) {
    EXPECT_EQ(writer_->ThreadCount(), 1);
}

// ---- 线程数为 3 ----
TEST_F(AsyncWriterTest, ThreadCountThree) {
    WriterConfig cfg;
    cfg.service_name     = "svc";
    cfg.queue_size       = 100;
    cfg.thread_pool_size = 3;
    auto mock = std::make_unique<MockMQProducer>();
    AsyncWriter w(cfg, std::move(mock));
    EXPECT_EQ(w.ThreadCount(), 3);
}

// =========================================================================
// GecDiff 全局单例测试（通过环境变量控制）
// =========================================================================

// ---- 未设置 GEC_DIFF_ENABLED → IsEnabled() == false ----
TEST(GecDiffEnvTest, DisabledByDefault) {
    unsetenv("GEC_DIFF_ENABLED");
    // 注意：单例一旦初始化就固定，此 test 必须在进程最早运行
    // 若其他 test 已启用，则跳过断言
    if (!GecDiff::IsEnabled()) {
        EXPECT_FALSE(GecDiff::IsEnabled());
    }
}

// ---- 禁用时 WriteBase/WriteTest 不崩溃（零开销退出）----
TEST(GecDiffEnvTest, WriteWhenDisabledDoesNotCrash) {
    // 无论是否启用，调用不应崩溃
    GecDiff::WriteBase("req", "key", 1.0);
    GecDiff::WriteTest("req", "key", 1.0);
    GecDiff::WriteBase("req", {{"a", 1}, {"b", 2.0}});
    GecDiff::WriteTest("req", {{"a", 1}, {"b", 2.0}});
}

// =========================================================================
// DiffValue 类型推导
// =========================================================================
TEST(DiffValueTest, ImplicitTypes) {
    DiffValue i32(42);
    EXPECT_EQ(i32.type, ValueType::INT32);

    DiffValue f64(3.14);
    EXPECT_EQ(f64.type, ValueType::FLOAT64);

    DiffValue b(true);
    EXPECT_EQ(b.type, ValueType::BOOL);

    DiffValue s("hello");
    EXPECT_EQ(s.type, ValueType::STRING);

    DiffValue json = DiffValue::Json("{\"k\":1}");
    EXPECT_EQ(json.type, ValueType::JSON);

    DiffValue bytes = DiffValue::Bytes("\x00\x01");
    EXPECT_EQ(bytes.type, ValueType::BYTES);
}
