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

class DiffClientTest : public ::testing::Test {
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
            .float_abs_epsilon = 1e-6,
            .float_rel_epsilon = 1e-4,
            .async_queue_size = 100,
        };

        writer_ = std::make_unique<AsyncWriter>(config_, std::move(mock_));
    }

    DiffConfig                    config_;
    std::unique_ptr<MockMQProducer> mock_;
    MockMQProducer*               mock_ptr_ = nullptr;
    std::unique_ptr<AsyncWriter>  writer_;
};

TEST_F(DiffClientTest, EnqueueAndFlush) {
    PendingMessage msg;
    msg.request_id   = "req_001";
    msg.service      = "test_service";
    msg.region       = "ROW";
    msg.side         = Side::OLD;
    msg.key          = "model_score";
    msg.fields       = {{"ctr_score", DiffValue::Float64(0.12)}};
    msg.tags         = {{"layer", "predict"}};
    msg.timestamp_ms = 1700000000000LL;

    EXPECT_TRUE(writer_->Enqueue(msg));

    msg.side = Side::NEW;
    msg.fields = {{"ctr_score", DiffValue::Float64(0.13)}};
    EXPECT_TRUE(writer_->Enqueue(msg));

    writer_->Flush();

    EXPECT_EQ(mock_ptr_->messages_.size(), 2u);
    EXPECT_EQ(mock_ptr_->messages_[0].topic, "test_service.diff.old");
    EXPECT_EQ(mock_ptr_->messages_[1].topic, "test_service.diff.new");
}

TEST_F(DiffClientTest, QueueFullDropsMessages) {
    DiffConfig small_cfg = config_;
    small_cfg.async_queue_size = 2;

    auto small_mock = std::make_unique<MockMQProducer>();
    AsyncWriter small_writer(small_cfg, std::move(small_mock));

    PendingMessage msg;
    msg.request_id = "req_x";
    msg.service    = "svc";
    msg.region     = "ROW";
    msg.side       = Side::OLD;
    msg.key        = "k";
    msg.timestamp_ms = 0;

    // 先把线程睡住使队列积压（通过发送大量消息）
    for (int i = 0; i < 10; ++i) {
        small_writer.Enqueue(msg);
    }

    EXPECT_GT(small_writer.DropCount(), 0u);
}

TEST_F(DiffClientTest, BatchEnqueue) {
    PendingMessage msg;
    msg.request_id = "req_batch";
    msg.service    = "test_service";
    msg.region     = "ROW";
    msg.side       = Side::OLD;
    msg.key        = "feature_map";
    msg.fields     = {
        {"ctr_score",   DiffValue::Float64(0.12)},
        {"cvr_score",   DiffValue::Float64(0.05)},
        {"final_score", DiffValue::Float64(0.089)},
    };
    msg.tags         = {{"layer", "model"}};
    msg.timestamp_ms = 1700000000000LL;

    EXPECT_TRUE(writer_->Enqueue(msg));
    writer_->Flush();
    EXPECT_EQ(mock_ptr_->messages_.size(), 1u);
}
