#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <numeric>
#include <optional>
#include <string>
#include <utils/print.h>
#include <utils/serde.h>

// Test fixture for serde tests
class SerdeTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

using namespace serde;

// ===== BaseDeserializationContext Tests =====

TEST_F(SerdeTest, DeserializationContextConstruction) {
  std::string data = "test data";
  std::string_view view = data;

  // Test construction from string_view
  DeserializationContext ctx1(view);
  EXPECT_EQ(ctx1.size(), data.size());
  EXPECT_EQ(std::string(ctx1), data);

  // Test construction from string
  DeserializationContext ctx2(data);
  EXPECT_EQ(ctx2.size(), data.size());
  EXPECT_EQ(std::string(ctx2), data);
}

TEST_F(SerdeTest, AssertPrefixSuccess) {
  std::string data = "prefix_rest_of_data";
  DeserializationContext ctx(data);

  EXPECT_TRUE(ctx.assert_prefix("prefix_"));
  EXPECT_EQ(std::string(ctx), "rest_of_data");
}

TEST_F(SerdeTest, AssertPrefixFailure) {
  std::string data = "data_without_expected_prefix";
  DeserializationContext ctx(data);

  EXPECT_FALSE(ctx.assert_prefix("wrong_"));
  EXPECT_EQ(std::string(ctx), data); // Should remain unchanged
}

TEST_F(SerdeTest, AssertPrefixEmpty) {
  std::string data = "some_data";
  DeserializationContext ctx(data);

  EXPECT_TRUE(ctx.assert_prefix(""));
  EXPECT_EQ(std::string(ctx), data); // Should remain unchanged
}

TEST_F(SerdeTest, AssertPrefixLongerThanData) {
  std::string data = "short";
  DeserializationContext ctx(data);

  EXPECT_FALSE(ctx.assert_prefix("much_longer_prefix"));
  EXPECT_EQ(std::string(ctx), data); // Should remain unchanged
}

// ===== BaseSerializationContext Tests =====

TEST_F(SerdeTest, SerializationContextConstruction) {
  std::string output;
  auto inserter = std::back_inserter(output);
  SerializationContext ctx(inserter);

  // Basic construction test - context should be valid
  EXPECT_NO_THROW(auto out = ctx.out());
}

TEST_F(SerdeTest, SerializationContextMoveSemantics) {
  std::string output;
  auto inserter = std::back_inserter(output);
  SerializationContext ctx1(inserter);

  // Test move constructor
  SerializationContext ctx2 = std::move(ctx1);
  EXPECT_NO_THROW(auto out = ctx2.out());

  // Test move assignment
  std::string output2;
  auto inserter2 = std::back_inserter(output2);
  SerializationContext ctx3(inserter2);
  ctx3 = std::move(ctx2);
  EXPECT_NO_THROW(auto out = ctx3.out());
}

// ===== Serializer/Deserializer Template Tests =====

TEST_F(SerdeTest, IntSerializationDeserialization) {
  constexpr int original = 42;

  // Test serialization
  std::string serialized = serialize(original);
  EXPECT_EQ(serialized, "42");

  // Test deserialization
  auto deserialized = deserialize<int>(serialized);
  ASSERT_TRUE(deserialized.has_value());
  EXPECT_EQ(deserialized.value(), original);
}

TEST_F(SerdeTest, IntDeserializationInvalid) {
  std::string invalid_data = "not_a_number";
  auto result = deserialize<int>(invalid_data);
  EXPECT_FALSE(result.has_value());
}

TEST_F(SerdeTest, StringSerializationDeserialization) {
  std::string original = "Hello, World!";

  // Test serialization
  std::string serialized = serialize(original);
  EXPECT_EQ(serialized, "\"Hello, World!\"");

  // Test deserialization
  auto deserialized = deserialize<std::string>(serialized);
  ASSERT_TRUE(deserialized.has_value());
  EXPECT_EQ(deserialized.value(), original);
}

TEST_F(SerdeTest, EmptyStringSerializationDeserialization) {
  std::string original;

  std::string serialized = serialize(original);
  EXPECT_EQ(serialized, "\"\"");

  auto deserialized = deserialize<std::string>(serialized);
  ASSERT_TRUE(deserialized.has_value());
  EXPECT_EQ(deserialized.value(), original);
}

TEST_F(SerdeTest, VectorIntSerializationDeserialization) {
  std::vector<int> original = {1, 2, 3, 42, -5};

  std::string serialized = serialize(original);
  EXPECT_EQ(serialized, "[5,1,2,3,42,-5]");

  auto deserialized = deserialize<std::vector<int>>(serialized);
  ASSERT_TRUE(deserialized.has_value());
  EXPECT_EQ(deserialized.value(), original);
}

TEST_F(SerdeTest, EmptyVectorSerializationDeserialization) {
  std::vector<int> original = {};

  std::string serialized = serialize(original);
  EXPECT_EQ(serialized, "[0]");

  auto deserialized = deserialize<std::vector<int>>(serialized);
  ASSERT_TRUE(deserialized.has_value());
  EXPECT_EQ(deserialized.value(), original);
}

// ===== Edge Cases and Error Handling =====

TEST_F(SerdeTest, DeserializationPartialConsumption) {
  std::string data = "42extra_data";
  DeserializationContext ctx(data);

  auto result = deserializer<int>::deserialize(ctx);
  EXPECT_TRUE(result.has_value());
  EXPECT_FALSE(ctx.empty());
}

TEST_F(SerdeTest, SerializationLargeData) {
  std::string large_string(10000, 'a');

  std::string serialized = serialize(large_string);
  auto deserialized = deserialize<std::string>(serialized);

  ASSERT_TRUE(deserialized.has_value());
  EXPECT_EQ(deserialized.value(), large_string);
}

TEST_F(SerdeTest, RoundTripConsistency) {
  // Test multiple round trips don't corrupt data
  std::string original = "test_data_123";

  for (int i = 0; i < 5; ++i) {
    std::string serialized = serialize(original);
    auto deserialized = deserialize<std::string>(serialized);

    ASSERT_TRUE(deserialized.has_value());
    EXPECT_EQ(deserialized.value(), original);

    original =
        deserialized.value(); // Use deserialized as input for next iteration
  }
}

// ===== Performance Tests (optional) =====

TEST_F(SerdeTest, SerializationPerformance) {
  std::vector<int> large_vector(1000);
  std::iota(large_vector.begin(), large_vector.end(), 1);

  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < 100; ++i) {
    std::string serialized = serialize(large_vector);
    // Prevent optimization
    volatile size_t size = serialized.size();
    (void)size;
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  // Just ensure it completes in reasonable time (adjust threshold as needed)
  EXPECT_LT(duration.count(), 100000); // Less than 100ms
}
