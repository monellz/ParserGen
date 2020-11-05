#include <gtest/gtest.h>
#include <string_view>
#include <unordered_set>

#define DBG_MACRO_DISABLE
#include "re.hpp"

using namespace parsergen::re;
using namespace parsergen::err;

TEST(basic, single_word_char) {
  std::unordered_set<std::string> words;
  for (char c = 'a'; c != 'z'; ++c) words.insert(std::string(1, c));
  for (char c = 'A'; c != 'Z'; ++c) words.insert(std::string(1, c));
  for (char c = '0'; c != '9'; ++c) words.insert(std::string(1, c));

  for (auto c: words) {
    auto [ptr, leaf_count] = ReEngine::produce(c);
    EXPECT_EQ(leaf_count, 1);
    auto res = ptr.release();
    ASSERT_NE(dynamic_cast<Concat*>(res), nullptr);
    auto concat = dynamic_cast<Concat*>(res);
    ASSERT_NE(dynamic_cast<Char*>(concat->sons[0]), nullptr);
    auto son = dynamic_cast<Char*>(concat->sons[0]);
    EXPECT_EQ(son->c, c[0]);
    delete res;
  }
}

TEST(basic, metachar) {
  EXPECT_NO_THROW({ auto _ = ReEngine::produce(R"(\w)"); });
  EXPECT_NO_THROW({ auto _ = ReEngine::produce(R"(\s)"); });
  EXPECT_NO_THROW({ auto _ = ReEngine::produce(R"(\n)"); });
  EXPECT_NO_THROW({ auto _ = ReEngine::produce(R"(a|b)"); });
  EXPECT_NO_THROW({ auto _ = ReEngine::produce(R"(\[\])"); });
}

TEST(basic, real_case) {
  EXPECT_NO_THROW({ auto _ = ReEngine::produce(R"([abc])"); });
  EXPECT_NO_THROW({ auto _ = ReEngine::produce(R"([^abc])"); });
  EXPECT_NO_THROW({ auto _ = ReEngine::produce(R"([\w])"); });
}
