#include <gtest/gtest.h>
#include "diff_value.h"

using namespace diff;

TEST(DiffValueTest, Int32RoundTrip) {
    DiffValue v = DiffValue::Int32(42);
    std::string bytes = v.Serialize();
    DiffValue v2 = DiffValue::Deserialize(ValueType::INT32, bytes);
    EXPECT_EQ(v2.i32, 42);
    EXPECT_EQ(v2.type, ValueType::INT32);
}

TEST(DiffValueTest, Float64RoundTrip) {
    DiffValue v = DiffValue::Float64(3.14159);
    std::string bytes = v.Serialize();
    DiffValue v2 = DiffValue::Deserialize(ValueType::FLOAT64, bytes);
    EXPECT_DOUBLE_EQ(v2.f64, 3.14159);
}

TEST(DiffValueTest, StringRoundTrip) {
    DiffValue v = DiffValue::String("hello_world");
    std::string bytes = v.Serialize();
    DiffValue v2 = DiffValue::Deserialize(ValueType::STRING, bytes);
    EXPECT_EQ(v2.str, "hello_world");
}

TEST(DiffValueTest, JsonRoundTrip) {
    std::string json = R"({"score":0.12,"tags":["vip"]})";
    DiffValue v = DiffValue::Json(json);
    std::string bytes = v.Serialize();
    DiffValue v2 = DiffValue::Deserialize(ValueType::JSON, bytes);
    EXPECT_EQ(v2.str, json);
    EXPECT_EQ(v2.type, ValueType::JSON);
}

TEST(DiffValueTest, BoolRoundTrip) {
    for (bool b : {true, false}) {
        DiffValue v = DiffValue::Bool(b);
        std::string bytes = v.Serialize();
        DiffValue v2 = DiffValue::Deserialize(ValueType::BOOL, bytes);
        EXPECT_EQ(v2.b, b);
    }
}

TEST(DiffValueTest, NullSerialize) {
    DiffValue v = DiffValue::Null();
    std::string bytes = v.Serialize();
    EXPECT_TRUE(bytes.empty());
}

TEST(DiffValueTest, TypeName) {
    EXPECT_STREQ(DiffValue::Float64(1.0).TypeName(), "float64");
    EXPECT_STREQ(DiffValue::Json("{}").TypeName(), "json");
    EXPECT_STREQ(DiffValue::Null().TypeName(), "null");
}

TEST(DiffValueTest, DebugString) {
    EXPECT_EQ(DiffValue::Int32(99).DebugString(), "int32(99)");
    EXPECT_EQ(DiffValue::Bool(true).DebugString(), "bool(true)");
    EXPECT_EQ(DiffValue::Null().DebugString(), "null");
}
