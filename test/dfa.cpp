#include <gtest/gtest.h>

#include <string_view>
#include <unordered_set>

//#define DBG_MACRO_DISABLE
#include "dfa.hpp"

using namespace parsergen::dfa;
using namespace parsergen::err;

TEST(set, set_cmp) {
  std::unordered_set<int> a = {1, 2, 3};
  std::unordered_set<int> b = {1, 2, 3};
  std::unordered_set<int> c;
  c.insert(3);
  c.insert(2);
  c.insert(1);
  std::unordered_set<int> d = {1, 2, 3, 4};
  EXPECT_EQ(a.size(), 3);
  EXPECT_EQ(b.size(), 3);
  EXPECT_EQ(a, b);
  EXPECT_EQ(a, c);
  EXPECT_NE(a, d);
}

TEST(map, auto_init) {
  std::unordered_map<int, std::string> m;
  EXPECT_EQ(m[0], "");
  EXPECT_EQ(m[1], "");
  EXPECT_EQ(m[2], "");
  EXPECT_EQ(m.size(), 3);
}

TEST(basic, single_char) {
  auto [_, char_dfa] = DfaEngine::produce("a");
  EXPECT_TRUE(char_dfa.accept("a"));
  EXPECT_FALSE(char_dfa.accept("b"));
  EXPECT_FALSE(char_dfa.accept("1"));
  EXPECT_FALSE(char_dfa.accept("\n"));
  EXPECT_FALSE(char_dfa.accept(" "));
  auto [__, num_dfa] = DfaEngine::produce("9");
  EXPECT_TRUE(num_dfa.accept("9"));
  EXPECT_FALSE(num_dfa.accept("1"));
  EXPECT_FALSE(num_dfa.accept("b"));
  EXPECT_FALSE(num_dfa.accept("\n"));
  EXPECT_FALSE(num_dfa.accept(" "));
}

TEST(basic, metachar) {
  auto [_, meta_dfa] = DfaEngine::produce(R"(\n)");
  EXPECT_TRUE(meta_dfa.accept("\n"));
  EXPECT_FALSE(meta_dfa.accept("\b"));
  EXPECT_FALSE(meta_dfa.accept("\\"));
  auto [__, plus_dfa] = DfaEngine::produce(R"(\+)");
  EXPECT_TRUE(plus_dfa.accept("+"));
  auto [___, dfa] = DfaEngine::produce(R"(a+)");
  EXPECT_TRUE(dfa.accept("a"));
  EXPECT_TRUE(dfa.accept("aa"));
  EXPECT_TRUE(dfa.accept("aaa"));
}

TEST(number, integer) {
  auto [_, single_dfa] = DfaEngine::produce(R"([0-9])");
  for (int i = 0; i <= 9; ++i) {
    EXPECT_TRUE(single_dfa.accept(std::to_string(i)));
  }
  EXPECT_FALSE(single_dfa.accept("asdadasd"));

  auto [__, int_dfa] = DfaEngine::produce(R"([1-9][0-9]*)");
  for (int i = 0; i < 1000; ++i) {
    int num = rand();
    ASSERT_TRUE(int_dfa.accept(std::to_string(num))) << num;
  }
  EXPECT_FALSE(int_dfa.accept("asdadasd"));
}

TEST(number, floating) {
  auto [_, float_dfa] = DfaEngine::produce(R"([-\+]?[0-9]*\.?[0-9]+)");
  EXPECT_TRUE(float_dfa.accept("1.123120220"));
  EXPECT_TRUE(float_dfa.accept("-0.0220"));
  EXPECT_TRUE(float_dfa.accept("-0.0"));
  EXPECT_TRUE(float_dfa.accept("+0"));
  EXPECT_TRUE(float_dfa.accept("0.2391"));
  EXPECT_TRUE(float_dfa.accept("12312312324238283.1"));
  EXPECT_TRUE(float_dfa.accept("12312312324238283.10100"));
  EXPECT_TRUE(float_dfa.accept("12312312324238283"));
}

TEST(real_case, const_integer) {
  auto [_, dfa] = DfaEngine::produce(R"(\d+|(0x[0-9a-fA-F]+))");
  EXPECT_TRUE(dfa.accept("0x123123"));
  EXPECT_TRUE(dfa.accept("123123"));
}

TEST(real_case, ident) {
  auto [_, dfa] = DfaEngine::produce(R"([_A-Za-z]\w*)");
  EXPECT_TRUE(dfa.accept("_"));
  EXPECT_TRUE(dfa.accept("asdasdasd"));
  EXPECT_TRUE(dfa.accept("number"));
  EXPECT_TRUE(dfa.accept("a1"));
  EXPECT_FALSE(dfa.accept("1a"));
}
