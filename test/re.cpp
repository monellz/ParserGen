#include <gtest/gtest.h>
#include <string_view>
#include <unordered_set>

#define DBG_MACRO_DISABLE
#include "re.hpp"

using namespace parsergen::re;
using namespace parsergen::err;

#define EXPECT_RE_TYPE(x, type) EXPECT_TRUE(std::holds_alternative<type>(x)) << "type is " << (x).index()

TEST(shared, equal) {
  auto re1 = std::make_shared<Re>(char('a'));
  auto re2 = re1;
  EXPECT_EQ(re1.use_count(), 2);
  EXPECT_EQ(re1, re2);
}

TEST(basic, single_word_char) {
  std::unordered_set<std::string> words;
  for (char c = 'a'; c != 'z'; ++c) words.insert(std::string(1, c));
  for (char c = 'A'; c != 'Z'; ++c) words.insert(std::string(1, c));
  for (char c = '0'; c != '9'; ++c) words.insert(std::string(1, c));

  for (auto c: words) {
    std::shared_ptr<Re> res;
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

TEST(basic, metachar) {
  EXPECT_NO_THROW({ auto res = parse(R"(\w)"); });
  EXPECT_NO_THROW({ auto res = parse(R"(\s)"); });
  EXPECT_NO_THROW({ auto res = parse(R"(\n)"); });
  EXPECT_NO_THROW({ auto res = parse(R"(a|b)"); });
  EXPECT_NO_THROW({ auto res = parse(R"(\[\])"); });
}

TEST(basic, real_case) {
  EXPECT_NO_THROW({ auto res = parse(R"([abc])"); });
  EXPECT_NO_THROW({ auto res = parse(R"([^abc])"); });
  EXPECT_NO_THROW({ auto res = parse(R"([\w])"); });
}

TEST(basic_err, unmatched) {
  EXPECT_THROW({
    auto res = parse("(a");
  }, ReErr);
  EXPECT_THROW({
    auto res = parse("(aada(");
  }, ReErr);
  EXPECT_THROW({
    auto res = parse("as();asdad)");
  }, ReErr);
  EXPECT_THROW({
    auto res = parse("([a)");
  }, ReErr);
}