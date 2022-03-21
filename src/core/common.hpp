#pragma once

#include <cassert>
#include <cstdint>
#include <functional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define DBG_MACRO_NO_WARNING
#include "dbg.h"
#include "err.hpp"

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

template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

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

// TODO: more efficient way(consume src_set)
template <typename T>
inline void union_inplace(std::unordered_set<T>& dst,
                          const std::unordered_set<T>& src) {
  for (auto c : src) dst.insert(c);
}

constexpr size_t FLOOR_LOG2(size_t x) {
  return x == 1 ? 0 : FLOOR_LOG2(x >> 1) + 1;
}
constexpr size_t CEIL_LOG2(size_t x) {
  return x == 1 ? 0 : FLOOR_LOG2(x - 1) + 1;
}

constexpr size_t LOG2(size_t x) { return CEIL_LOG2(x); }

template <typename T>
class IntSet {
 private:
  T* _data;
  size_t _size;
  size_t _capacity;

 public:
  static constexpr size_t _TSIZE = 8 * sizeof(T);
  static constexpr size_t _BITLOG = LOG2(_TSIZE);
  static constexpr T _MASK = _TSIZE - 1;

  IntSet(size_t elem_num) {
    _capacity = (elem_num + _TSIZE - 1) / _TSIZE;
    _data = new T[_capacity];
    memset(_data, 0, sizeof(T) * _capacity);
  }

  bool operator==(const IntSet& other) const {
    for (size_t i = 0; i < _size; ++i) {
      if (_data[i] != other._data[i]) return false;
    }
    return true;
  }

  void insert(size_t val) {
    assert((val >> _BITLOG) < _capacity);
    _data[val >> _BITLOG] |= (1 << (val & _MASK));
  }

  void erase(size_t val) {
    assert((val >> _BITLOG) < _capacity);
    _data[val >> _BITLOG] &= ~(1 << (val & _MASK));
  }

  bool check(size_t val) {
    assert((val >> _BITLOG) < _capacity);
    return (_data[val >> _BITLOG] & (1 << (val & _MASK))) != 0;
  }

  void union_inplace(const IntSet& other) {}

  ~IntSet() { delete[] _data; }
};

}  // namespace parsergen
