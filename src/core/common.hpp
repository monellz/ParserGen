#pragma once

#include <vector>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <cassert>
#include <cstdint>

#define DBG_MACRO_NO_WARNING
#include "dbg.h"
#include "err.hpp"

namespace parsergen {

using u8 = uint8_t;
using u32 = uint32_t;
using i32 = int32_t;

#define ERR_EXIT(...) \
  do { \
    dbg(__VA_ARGS__); \
    exit(-1);  \
  } while (false)

#define UNREACHABLE() ERR_EXIT("control flow should never reach here")


template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

inline std::vector<std::string_view> split(std::string_view sv, std::string_view delims) {
  std::vector<std::string_view> output;
  for (auto first = sv.data(), second = sv.data(), last = first + sv.size(); second != last && first != last; first = second + 1) {
      second = std::find_first_of(first, last, std::cbegin(delims), std::cend(delims));
      if (first != second) output.emplace_back(first, second - first);
  }
  return output;
}

// TODO: more efficient way(consume src_set)
template<typename T>
inline void union_inplace(std::unordered_set<T>& dst, const std::unordered_set<T>& src) {
  for (auto c: src) dst.insert(c);
}


}  // namespace parsergen