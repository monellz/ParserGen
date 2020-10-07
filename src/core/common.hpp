#pragma once

#include <vector>
#include <string_view>

#define DBG_MACRO_NO_WARNING
#include "dbg.h"
#include "err.hpp"

#define ERR_EXIT(...) \
  do { \
    dbg(__VA_ARGS__); \
    exit(-1);  \
  } while (false)

#define UNREACHABLE() ERR_EXIT("control flow should never reach here")


inline std::vector<std::string_view> split(std::string_view sv, std::string_view delims) {
  std::vector<std::string_view> output;
  for (auto first = sv.data(), second = sv.data(), last = first + sv.size(); second != last && first != last; first = second + 1) {
      second = std::find_first_of(first, last, std::cbegin(delims), std::cend(delims));
      if (first != second) output.emplace_back(first, second - first);
  }
  return output;
}

