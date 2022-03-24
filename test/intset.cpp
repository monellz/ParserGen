#include <gtest/gtest.h>

#include <string_view>
#include <unordered_set>

//#define DBG_MACRO_DISABLE
#include "common.hpp"

using namespace parsergen;

TEST(basic_func, const_log2) {
  // EXPECT_EQ(LOG2(1), 1);
  EXPECT_EQ(LOG2(1), 0);

  EXPECT_EQ(LOG2(2), 1);
  EXPECT_EQ(LOG2(3), 2);
  EXPECT_EQ(LOG2(4), 2);
  for (int i = 5; i <= 8; ++i) EXPECT_EQ(LOG2(i), 3);
  for (int i = 9; i <= 16; ++i) EXPECT_EQ(LOG2(i), 4);
  for (int i = 17; i <= 32; ++i) EXPECT_EQ(LOG2(i), 5);
  for (int i = 33; i <= 64; ++i) EXPECT_EQ(LOG2(i), 6);
}

TEST(intset, attr) {
  EXPECT_EQ(IntSet<uint8_t>::_TSIZE, 8);
  EXPECT_EQ(IntSet<uint16_t>::_TSIZE, 16);
  EXPECT_EQ(IntSet<uint32_t>::_TSIZE, 32);
  EXPECT_EQ(IntSet<uint64_t>::_TSIZE, 64);

  EXPECT_EQ(IntSet<uint8_t>::_BITLOG, 3);
  EXPECT_EQ(IntSet<uint16_t>::_BITLOG, 4);
  EXPECT_EQ(IntSet<uint32_t>::_BITLOG, 5);
  EXPECT_EQ(IntSet<uint64_t>::_BITLOG, 6);

  EXPECT_EQ(IntSet<uint8_t>::_MASK, 7);
  EXPECT_EQ(IntSet<uint16_t>::_MASK, 15);
  EXPECT_EQ(IntSet<uint32_t>::_MASK, 31);
  EXPECT_EQ(IntSet<uint64_t>::_MASK, 63);
}

TEST(intset_8, op) {
  IntSet<uint8_t> s(1000);
  for (size_t i = 0; i < 1000; ++i) {
    s.insert(i);
    EXPECT_TRUE(s.check(i));
    s.erase(i);
    EXPECT_FALSE(s.check(i));
  }
}

TEST(intset_16, op) {
  IntSet<uint16_t> s(1000);
  for (size_t i = 0; i < 1000; ++i) {
    s.insert(i);
    EXPECT_TRUE(s.check(i));
    s.erase(i);
    EXPECT_FALSE(s.check(i));
  }
}

TEST(intset_32, op) {
  IntSet<uint32_t> s(1000);
  for (size_t i = 0; i < 1000; ++i) {
    s.insert(i);
    EXPECT_TRUE(s.check(i));
    s.erase(i);
    EXPECT_FALSE(s.check(i));
  }
}

TEST(intset_64, op) {
  IntSet<uint64_t> s(1000);
  for (size_t i = 0; i < 1000; ++i) {
    s.insert(i);
    EXPECT_TRUE(s.check(i));
    s.erase(i);
    EXPECT_FALSE(s.check(i));
  }
}
