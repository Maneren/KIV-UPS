#include <array>
#include <cstdint>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <optional>
#include <string>
#include <utility>
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

// ===== AssertPrefix Edge Cases =====

TEST_F(SerdeTest, AssertPrefixTruncatedInput) {
  // Remaining input is a strict prefix of the needle: must fail without
  // reading past the end of the input (previously an out-of-bounds read).
  std::string data = "HEL";
  DeserializationContext ctx(data);

  EXPECT_FALSE(ctx.assert_prefix("HELLO WORLD, THIS IS LONG"));
  EXPECT_EQ(std::string(ctx), "HEL");
}

TEST_F(SerdeTest, AssertPrefixExactMatch) {
  std::string data = "HELLO";
  DeserializationContext ctx(data);

  EXPECT_TRUE(ctx.assert_prefix("HELLO"));
  EXPECT_TRUE(ctx.empty());
}

// ===== Keyword Table Tests =====

namespace {

enum class TestKeyword : std::uint8_t {
  Alpha,
  Beta,
  Gamma,
};

constexpr std::pair<std::string_view, TestKeyword> TEST_KEYWORD_TABLE[] = {
    {"ALPHA", TestKeyword::Alpha},
    {"BETA", TestKeyword::Beta},
    {"GAMMA", TestKeyword::Gamma},
};

std::string write_test_keyword(TestKeyword value) {
  std::string out;
  auto it = std::back_inserter(out);
  SerializationContext ctx(it);
  write_keyword(value, ctx.out(), TEST_KEYWORD_TABLE);
  return out;
}

} // namespace

template <> struct serde::deserializer<TestKeyword> {
  static std::optional<TestKeyword> deserialize(auto &ctx) {
    return serde::read_keyword<TestKeyword>(ctx, TEST_KEYWORD_TABLE);
  }
};

TEST_F(SerdeTest, ChainedWritesThreadPosition) {
  // std::format_to parity: the returned iterator is authoritative, so
  // chained writes append even into single-pass sinks (here a raw
  // buffer) that cannot re-query their position.
  std::array<char, 16> buf{};
  SerializationContext<char *> ctx(buf.data());
  auto it = ctx.out();
  it = write_keyword(TestKeyword::Alpha, it, TEST_KEYWORD_TABLE);
  it = write_keyword(TestKeyword::Beta, it, TEST_KEYWORD_TABLE);
  EXPECT_EQ(it, buf.data() + 9);
  EXPECT_EQ(std::string_view(buf.data(), 9), "ALPHABETA");
}

// ===== DeserializeFull Tests =====

TEST_F(SerdeTest, DeserializeFullAcceptsExactInput) {
  const auto result = deserialize_full<TestKeyword>(std::string_view{"BETA"});
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, TestKeyword::Beta);
}

TEST_F(SerdeTest, DeserializeFullRejectsTrailingGarbage) {
  EXPECT_FALSE(
      deserialize_full<TestKeyword>(std::string_view{"BETA!"}).has_value()
  );
  EXPECT_FALSE(deserialize_full<TestKeyword>(std::string_view{""}).has_value());
}

TEST_F(SerdeTest, ReadKeywordMatch) {
  DeserializationContext ctx("BETA rest");
  const auto result = read_keyword<TestKeyword>(ctx, TEST_KEYWORD_TABLE);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, TestKeyword::Beta);
  EXPECT_EQ(std::string(ctx), " rest");
}

TEST_F(SerdeTest, ReadKeywordNoMatchLeavesContextUntouched) {
  DeserializationContext ctx("DELTA rest");
  EXPECT_FALSE(read_keyword<TestKeyword>(ctx, TEST_KEYWORD_TABLE).has_value());
  EXPECT_EQ(std::string(ctx), "DELTA rest");
}

TEST_F(SerdeTest, WriteKeywordRoundTrip) {
  EXPECT_EQ(write_test_keyword(TestKeyword::Alpha), "ALPHA");
  EXPECT_EQ(write_test_keyword(TestKeyword::Gamma), "GAMMA");

  DeserializationContext ctx(write_test_keyword(TestKeyword::Beta));
  const auto result = read_keyword<TestKeyword>(ctx, TEST_KEYWORD_TABLE);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, TestKeyword::Beta);
  EXPECT_TRUE(ctx.empty());
}

// ===== DeserializationTransaction Tests =====

TEST_F(SerdeTest, TransactionRestoresOnFailure) {
  DeserializationContext ctx("LOGIN 05alice");
  {
    DeserializationTransaction tx(ctx);
    EXPECT_TRUE(ctx.assert_prefix("LOGIN "));
    EXPECT_TRUE(ctx.assert_prefix("05"));
    // Simulate a failed multi-step parse: no commit.
  }
  EXPECT_EQ(std::string(ctx), "LOGIN 05alice");
}

TEST_F(SerdeTest, TransactionKeepsProgressOnCommit) {
  DeserializationContext ctx("LOGIN 05alice");
  {
    DeserializationTransaction tx(ctx);
    EXPECT_TRUE(ctx.assert_prefix("LOGIN "));
    tx.commit();
  }
  EXPECT_EQ(std::string(ctx), "05alice");
}
