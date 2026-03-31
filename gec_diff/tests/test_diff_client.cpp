#include <gtest/gtest.h>
#include "diff_client.h"
#include "async_writer.h"

#include <vector>
#include <mutex>

using namespace diff;

// ---- Mock MQ Producer ----
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

class DiffScopeTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_ = std::make_unique<MockMQProducer>();
        mock_ptr_ = mock_.get();
        config_ = DiffConfig{
            .mq_endpoint      = "mock://",
            .service_name     = "test_service",
            .region           = "ROW",
            .sample_rate      = 1.0f,
            .diff_mode        = DiffMode::STRICT,
            .async_queue_size = 100,
            .thread_pool_size = 1,  // 测试用单线程
        };
        writer_ = std::make_unique<AsyncWriter>(config_, std::move(mock_));
    }
    DiffConfig config_;
    std::unique_ptr<MockMQProducer> mock_;
    MockMQProducer* mock_ptr_ = nullptr;
    std::unique_ptr<AsyncWriter> writer_;
};

// ---- 测试：Write 使用 Group::BASE / Group::TEST ----
TEST_F(DiffScopeTest, GroupTopicRouting) {
    PendingMessage base_msg;
    base_msg.request_id = "req_001"; base_msg.service = "test_service";
    base_msg.region = "ROW"; base_msg.group = Group::BASE;
    base_msg.key = "ctr_score"; base_msg.fields = {{"", 0.12}};
    base_msg.timestamp_ms = 0;

    PendingMessage test_msg = base_msg;
    test_msg.group = Group::TEST;
    test_msg.fields = {{"", 0.13}};

    EXPECT_TRUE(writer_->Enqueue(base_msg));
    EXPECT_TRUE(writer_->Enqueue(test_msg));
    writer_->Flush();

    EXPECT_EQ(mock_ptr_->messages_.size(), 2u);
    // BASE → topic .diff.base，TEST → .diff.test
    EXPECT_EQ(mock_ptr_->messages_[0].topic, "test_service.diff.base");
    EXPECT_EQ(mock_ptr_->messages_[1].topic, "test_service.diff.test");
}

// ---- 测试：隐式类型构造（FieldList 无需手动指定 DiffValue 类型）----
TEST_F(DiffScopeTest, ImplicitTypeConversion) {
    PendingMessage msg;
    msg.request_id = "req_002"; msg.service = "test_service";
    msg.region = "ROW"; msg.group = Group::BASE; msg.key = "feature_map";
    msg.fields = {
        {"ctr",     0.12},       // double  → FLOAT64
        {"uid",     12345},      // int     → INT32
        {"is_vip",  true},       // bool    → BOOL
        {"name",    "alice"},    // string  → STRING
    };
    msg.timestamp_ms = 0;

    EXPECT_TRUE(writer_->Enqueue(msg));
    writer_->Flush();
    EXPECT_EQ(mock_ptr_->messages_.size(), 1u);
}

// ---- 测试：队列满时丢弃 ----
TEST_F(DiffScopeTest, QueueFullDropsMessages) {
    DiffConfig small_cfg = config_;
    small_cfg.async_queue_size = 2;
    auto small_mock = std::make_unique<MockMQProducer>();
    AsyncWriter small_writer(small_cfg, std::move(small_mock));

    PendingMessage msg;
    msg.request_id = "req_x"; msg.service = "svc"; msg.region = "ROW";
    msg.group = Group::BASE; msg.key = "k"; msg.timestamp_ms = 0;
    for (int i = 0; i < 10; ++i) small_writer.Enqueue(msg);
    EXPECT_GT(small_writer.DropCount(), 0u);
}

// ---- 测试：线程池大小从 config 读取 ----
TEST_F(DiffScopeTest, ThreadPoolSizeFromConfig) {
    EXPECT_EQ(writer_->ThreadCount(), 1);
}

// ---- 测试：线程池大小从环境变量读取 ----
TEST_F(DiffScopeTest, ThreadPoolSizeFromEnv) {
    DiffConfig cfg = config_;
    cfg.thread_pool_size = 0;  // 让它走环境变量
    setenv("GEC_DIFF_THREAD_POOL_SIZE", "3", 1);
    {
        auto env_mock = std::make_unique<MockMQProducer>();
        AsyncWriter env_writer(cfg, std::move(env_mock));
        EXPECT_EQ(env_writer.ThreadCount(), 3);
    }
    unsetenv("GEC_DIFF_THREAD_POOL_SIZE");
}
