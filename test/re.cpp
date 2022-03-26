#include <gtest/gtest.h>

#include <set>
#include <string_view>
#include <unordered_set>

//#define DBG_MACRO_DISABLE
#include "core/re.h"
#include "util/casting.h"

using namespace parsergen;
using namespace parsergen::re;

TEST(basic, single_word_char) {
  std::set<std::string> words;
  for (char c = 'a'; c != 'z'; ++c) words.insert(std::string(1, c));
  for (char c = 'A'; c != 'Z'; ++c) words.insert(std::string(1, c));
  for (char c = '0'; c != '9'; ++c) words.insert(std::string(1, c));

  for (auto c : words) {
    auto ptr = Re::parse(c);
    EXPECT_TRUE(isa<Concat>(ptr));
    auto concat = static_cast<Concat*>(ptr.get());
    EXPECT_TRUE(isa<Char>(concat->sons[0]));
    auto son = static_cast<Char*>(concat->sons[0].get());
    EXPECT_EQ(son->c, c[0]);
  }
}

TEST(basic, metachar) {
  EXPECT_NO_THROW({ auto _ = Re::parse(R"(\w)"); });
  EXPECT_NO_THROW({ auto _ = Re::parse(R"(\s)"); });
  EXPECT_NO_THROW({ auto _ = Re::parse(R"(\n)"); });
  EXPECT_NO_THROW({ auto _ = Re::parse(R"(a|b)"); });
  EXPECT_NO_THROW({ auto _ = Re::parse(R"(\[\])"); });
}

TEST(basic, real_case) {
  EXPECT_NO_THROW({ auto _ = Re::parse(R"([abc])"); });
  EXPECT_NO_THROW({ auto _ = Re::parse(R"([^abc])"); });
  EXPECT_NO_THROW({ auto _ = Re::parse(R"([\w])"); });
}
