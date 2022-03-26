#ifndef __COMMON_H
#define __COMMON_H

#include <cassert>
#include <cstdint>
#include <functional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define DBG_MACRO_NO_WARNING
#include "dbg.h"

namespace parsergen {

using u8 = uint8_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i32 = int32_t;

#define ERR_EXIT(...) \
  do {                \
    dbg(__VA_ARGS__); \
    exit(-1);         \
  } while (false)

#define UNREACHABLE() ERR_EXIT("control flow should never reach here")

inline std::vector<std::string_view> split(std::string_view sv,
                                           std::string_view delims) {
  std::vector<std::string_view> output;
  for (auto first = sv.data(), second = sv.data(), last = first + sv.size();
       second != last && first != last; first = second + 1) {
    second =
        std::find_first_of(first, last, std::cbegin(delims), std::cend(delims));
    if (first != second) output.emplace_back(first, second - first);
  }
  return output;
}

}  // namespace parsergen

#endif
