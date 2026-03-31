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
// AsyncWriter 单元测试
// =========================================================================
class AsyncWriterTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_ptr_ = new MockMQProducer();
        WriterConfig cfg;
        cfg.service_name     = "test_service";
        cfg.queue_size       = 100;
        cfg.thread_pool_size = 1;
        writer_ = std::make_unique<AsyncWriter>(cfg,
                      std::unique_ptr<MockMQProducer>(mock_ptr_));
    }
    MockMQProducer*          mock_ptr_ = nullptr;
    std::unique_ptr<AsyncWriter> writer_;
};

// ---- BASE/TEST 路由到各自 topic ----
TEST_F(AsyncWriterTest, GroupTopicRouting) {
    PendingMessage base_msg;
    base_msg.request_id   = "req_001";
    base_msg.service      = "test_service";
    base_msg.region       = "ROW";
    base_msg.group        = Group::BASE;
    base_msg.key          = "ctr_score";
    base_msg.fields       = {{"", DiffValue(0.12)}};
    base_msg.timestamp_ms = 0;

    PendingMessage test_msg = base_msg;
    test_msg.group  = Group::TEST;
    test_msg.fields = {{"", DiffValue(0.13)}};

    EXPECT_TRUE(writer_->Enqueue(base_msg));
    EXPECT_TRUE(writer_->Enqueue(test_msg));
    writer_->Flush();

    EXPECT_EQ(mock_ptr_->messages_.size(), 2u);
    EXPECT_EQ(mock_ptr_->messages_[0].topic, "test_service.diff.base");
    EXPECT_EQ(mock_ptr_->messages_[1].topic, "test_service.diff.test");
}

// ---- 队列满时丢弃 ----
TEST_F(AsyncWriterTest, QueueFullDropsMessages) {
    auto* small_mock = new MockMQProducer();
    WriterConfig small_cfg;
    small_cfg.service_name     = "svc";
    small_cfg.queue_size       = 2;
    small_cfg.thread_pool_size = 1;
    AsyncWriter small_writer(small_cfg,
                              std::unique_ptr<MockMQProducer>(small_mock));

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

// ---- 线程数 ----
TEST_F(AsyncWriterTest, ThreadCount) {
    EXPECT_EQ(writer_->ThreadCount(), 1);
}

// =========================================================================
// 类型转换层：DiffValue 隐式构造
// =========================================================================
TEST(DiffValueTest, ImplicitTypes) {
    EXPECT_EQ(DiffValue(42).type,      ValueType::INT32);
    EXPECT_EQ(DiffValue(3.14).type,    ValueType::FLOAT64);
    EXPECT_EQ(DiffValue(true).type,    ValueType::BOOL);
    EXPECT_EQ(DiffValue("hi").type,    ValueType::STRING);
    EXPECT_EQ(DiffValue::Json("{}").type,    ValueType::JSON);
    EXPECT_EQ(DiffValue::Bytes("\x00").type, ValueType::BYTES);
}

// =========================================================================
// GecDiff::Write 不崩溃（未启用时零开销退出）
// =========================================================================
TEST(GecDiffTest, WriteWhenDisabledDoesNotCrash) {
    // 模板路径：T → DiffValue 转换 + MQ 写入分开调用
    diff::Write(Group::BASE, "req", "key", 0.12);
    diff::Write(Group::TEST, "req", "key", true);
    diff::Write(Group::BASE, "req", "key", std::string("val"));

    // 批量路径：用户自己构造 DiffValue
    diff::Write(Group::BASE, "req", FieldList{{"a", DiffValue(1)}, {"b", DiffValue(2.0)}});
}

// =========================================================================
// 用户调用示例（模拟真实业务场景）
// =========================================================================

// 场景 1：推荐系统 CTR 打分，对照组 vs 实验组单值对比
TEST(UserUsageTest, SingleValueCtrScore) {
    std::string request_id = "rec_req_abc123";
    Tags tags = {{"scene", "homepage"}, {"uid", "10086"}};

    // 旧模型打分（对照组）
    double old_score = 0.312;
    diff::Write(Group::BASE, request_id, "ctr_score", old_score, tags);

    // 新模型打分（实验组）
    double new_score = 0.328;
    diff::Write(Group::TEST, request_id, "ctr_score", new_score, tags);
}

// 场景 2：特征工程迁移，批量 diff 多个特征值
TEST(UserUsageTest, BatchFeatureMapDiff) {
    std::string request_id = "feat_req_xyz789";
    Tags tags = {{"region", "ROW"}};

    // 旧特征抽取结果
    diff::Write(Group::BASE, request_id, FieldList{
        {"age_bucket",  DiffValue(3)},
        {"city_level",  DiffValue(1)},
        {"is_new_user", DiffValue(false)},
        {"item_price",  DiffValue(99.9)},
    }, tags);

    // 新特征抽取结果
    diff::Write(Group::TEST, request_id, FieldList{
        {"age_bucket",  DiffValue(3)},
        {"city_level",  DiffValue(2)},      // 变化
        {"is_new_user", DiffValue(false)},
        {"item_price",  DiffValue(99.9)},
    }, tags);
}

// 场景 3：下游服务返回 JSON 结构体对比
TEST(UserUsageTest, JsonResponseDiff) {
    std::string request_id = "api_req_def456";
    Tags tags = {{"caller", "rank_service"}};

    std::string old_resp = R"({"code":0,"items":[1,2,3]})";
    std::string new_resp = R"({"code":0,"items":[1,2,4]})";

    GecDiff::Write(Group::BASE, request_id, "rank_result",
                   DiffValue::Json(old_resp), tags);
    GecDiff::Write(Group::TEST, request_id, "rank_result",
                   DiffValue::Json(new_resp), tags);
}

// 场景 4：不传 tags（可选参数缺省）
TEST(UserUsageTest, WriteWithoutTags) {
    diff::Write(Group::BASE, "req_notag", "score", 0.5f);
    diff::Write(Group::TEST, "req_notag", "score", 0.6f);
}
