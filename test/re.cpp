#include <gtest/gtest.h>
#include <string_view>
#include <unordered_set>
#include "re.hpp"

using namespace parsergen::re;

#define EXPECT_RE_TYPE(x, type) EXPECT_TRUE(std::holds_alternative<type>(x)) << "type is " << (x).index()

TEST(basic, single_word_char) {
  std::unordered_set<std::string> words;
  for (char c = 'a'; c != 'z'; ++c) words.insert(std::string(1, c));
  for (char c = 'A'; c != 'Z'; ++c) words.insert(std::string(1, c));
  for (char c = '0'; c != '9'; ++c) words.insert(std::string(1, c));

  for (auto c: words) {
    std::unique_ptr<Re> res;
    EXPECT_NO_THROW({
      res = parse(c);
    });
    EXPECT_RE_TYPE(*res, Concat);
    auto& vec = std::get<Concat>(*res).inners;
    auto& p = vec[0];
    EXPECT_RE_TYPE(*p, char);
    EXPECT_EQ(std::get<char>(*p), c[0]);
  }
}