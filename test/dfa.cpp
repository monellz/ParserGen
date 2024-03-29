#include <gtest/gtest.h>

#include <string_view>
#include <unordered_set>

//#define DBG_MACRO_DISABLE
#include "core/dfa.h"

using namespace parsergen::dfa;

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
  auto char_dfa = Dfa::from_sv("a");
  EXPECT_TRUE(char_dfa.accept("a"));
  EXPECT_FALSE(char_dfa.accept("b"));
  EXPECT_FALSE(char_dfa.accept("1"));
  EXPECT_FALSE(char_dfa.accept("\n"));
  EXPECT_FALSE(char_dfa.accept(" "));
  auto num_dfa = Dfa::from_sv("9");
  EXPECT_TRUE(num_dfa.accept("9"));
  EXPECT_FALSE(num_dfa.accept("1"));
  EXPECT_FALSE(num_dfa.accept("b"));
  EXPECT_FALSE(num_dfa.accept("\n"));
  EXPECT_FALSE(num_dfa.accept(" "));
}

TEST(basic, metachar) {
  auto meta_dfa = Dfa::from_sv(R"(\n)");
  EXPECT_TRUE(meta_dfa.accept("\n"));
  EXPECT_FALSE(meta_dfa.accept("\b"));
  EXPECT_FALSE(meta_dfa.accept("\\"));
  auto plus_dfa = Dfa::from_sv(R"(\+)");
  EXPECT_TRUE(plus_dfa.accept("+"));
  auto dfa = Dfa::from_sv(R"(a+)");
  EXPECT_TRUE(dfa.accept("a"));
  EXPECT_TRUE(dfa.accept("aa"));
  EXPECT_TRUE(dfa.accept("aaa"));
}

TEST(number, integer) {
  auto single_dfa = Dfa::from_sv(R"([0-9])");
  for (int i = 0; i <= 9; ++i) {
    EXPECT_TRUE(single_dfa.accept(std::to_string(i)));
  }
  EXPECT_FALSE(single_dfa.accept("asdadasd"));

  auto int_dfa = Dfa::from_sv(R"([1-9][0-9]*)");
  for (int i = 0; i < 1000; ++i) {
    int num = rand();
    ASSERT_TRUE(int_dfa.accept(std::to_string(num))) << num;
  }
  EXPECT_FALSE(int_dfa.accept("asdadasd"));
}

TEST(number, floating) {
  auto float_dfa = Dfa::from_sv(R"([-+]?[0-9]*[.][0-9]*([eE][-+]?[0-9]+)?)");
  EXPECT_TRUE(float_dfa.accept("1.123120220"));
  EXPECT_TRUE(float_dfa.accept("-0.0220"));
  EXPECT_TRUE(float_dfa.accept("-0.0"));
  EXPECT_TRUE(float_dfa.accept("+0.123123"));
  EXPECT_TRUE(float_dfa.accept("0.2391"));
  EXPECT_TRUE(float_dfa.accept("12312312324238283.1"));
  EXPECT_TRUE(float_dfa.accept("12312312324238283.10100"));
  EXPECT_TRUE(float_dfa.accept("-.123123"));
  EXPECT_TRUE(float_dfa.accept("-.1231e1234"));
  EXPECT_TRUE(float_dfa.accept("-0.1231e1234"));
  EXPECT_TRUE(float_dfa.accept("0.1231e1234"));
  EXPECT_TRUE(float_dfa.accept("-.1231E1234"));
  EXPECT_TRUE(float_dfa.accept("-.1231E+1234"));
  EXPECT_TRUE(float_dfa.accept("-.1231E-1234"));
  EXPECT_TRUE(float_dfa.accept("+.1231E-1234"));

  EXPECT_FALSE(float_dfa.accept("12312312324238283"));
}

TEST(real_case, const_integer) {
  auto dfa = Dfa::from_sv(R"(\d+|(0x[0-9a-fA-F]+))");
  EXPECT_TRUE(dfa.accept("0x123123"));
  EXPECT_TRUE(dfa.accept("123123"));
}

TEST(real_case, ident) {
  auto dfa = Dfa::from_sv(R"([_A-Za-z]\w*)");
  EXPECT_TRUE(dfa.accept("_"));
  EXPECT_TRUE(dfa.accept("asdasdasd"));
  EXPECT_TRUE(dfa.accept("number"));
  EXPECT_TRUE(dfa.accept("a1"));
  EXPECT_FALSE(dfa.accept("1a"));
}
