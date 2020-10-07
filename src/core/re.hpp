#pragma once

#include <memory>
#include <string_view>
#include <vector>
#include <variant>
#include <unordered_set>

#include "common.hpp"

namespace parsergen::re {

struct Eps {};
struct Kleene;
struct Plus;
struct Concat;
struct Disjunction;
using Re = std::variant<Eps, char, Kleene, Plus, Concat, Disjunction>;

struct Kleene {
  std::unique_ptr<Re> inner;
};

struct Plus {
  std::unique_ptr<Re> inner;
};

struct Concat {
  std::vector<std::unique_ptr<Re>> inners;
};
struct Disjunction {
  std::vector<std::unique_ptr<Re>> inners;
};



std::unique_ptr<Re> parse_brackets(std::string_view sv) {
  std::string_view original_sv = sv;

  std::unordered_set<char> hs;
  hs.reserve(256);

  std::function<void(char)> update;
  if (sv[0] == '^') {
    for (int i = 0; i < 256; ++i) hs.insert(char(i));
    sv.remove_prefix(1);

    update = [&](char c) { hs.erase(c); };
  } else {
    update = [&](char c) { hs.insert(c); };
  }

  std::vector<std::string_view> output = split(sv, "-");
  for (int i = 0; i < output.size() - 1; ++i) {
    if (output[i].empty()) continue;
    if (output[i + 1].empty()) {
      dbg(original_sv);
      throw err::ReErr(original_sv, "range not avlid: left/right not match");
      output[i].remove_suffix(1);
      continue;
    }
    int start = int(output[i].back());
    int end = int(output[i + 1].front());
    if (end < start) {
      dbg(original_sv);
      throw err::ReErr(original_sv, "range not valid: end < start");
    } else {
      // valid
      for (int j = start; j <= end; ++j) {
        update(char(j));
      }
    }
    output[i].remove_suffix(1);
    output[i + 1].remove_prefix(1);
  }

  // parse normal char
  for (auto s: output) {
    while (!s.empty()) {
      switch (s[0]) {
        case '\\': {
          // meta char
          if (s.size() == 1) {
            dbg(original_sv);
            throw err::ReErr(original_sv, "escaped char is not complete");
            s.remove_prefix(1);
            break;
          }
          switch (s[1]) {
            case '\\':
            case '(':
            case ')':
            case '[':
            case ']':
            case '.':
            case '|':
            case '*':
            case '+':
            case '?':
            case '{':
            case '}':
            case '^':
            case '$': {
              // escape
              update(sv[1]);
              break;
            }
            case 'n': {
              update('\n');
              break;
            }
            case 't': {
              update('\t');
              break;
            }
            case 's': {
              update('\n');
              update('\t');
              update('\r');
              break;
            }
            case 'w': {
              //[A-Za-z0-9_]
              for (char c = 'A'; c <= 'Z'; ++c) update(c);
              for (char c = 'a'; c <= 'z'; ++c) update(c);
              for (char c = '0'; c <= '9'; ++c) update(c);
              update('_');
              break;
            }
            default: {
              dbg(original_sv);
              throw err::ReErr(original_sv, "unsupported char for escaping: " + std::string(s.substr(0, 2)));
              break;
            }
          }
          s.remove_prefix(2);
          break;
        }
        case '(':
        case ')':
        case '[':
        case ']':
        case '.':
        case '|':
        case '*':
        case '+':
        case '?':
        case '{':
        case '}':
        case '^':
        case '$': {
          throw err::ReErr(original_sv, "not support unescaped metachar in brackets");
          s.remove_prefix(1);
          break;
        }
        default: {
          // normal char
          update(s[0]);
          s.remove_prefix(1);
          break;
        }
      }
    }
  }

  auto res = new Re(Disjunction());
  auto& inners = std::get<Disjunction>(*res).inners;
  for (auto c: hs) {
    auto re = std::make_unique<Re>(Re(char(c)));
    inners.push_back(std::move(re));
  }

  return std::unique_ptr<Re>(res);
}


std::unique_ptr<Re> parse_without_pipe(std::string_view sv) {
  // meta char
  // ()[].|*+\?  not support {} ^ $

  // stack elem use Concat
  std::vector<std::unique_ptr<Re>> stack;
  std::string_view original_sv = sv;
  while (!sv.empty()) {
    switch (sv[0]) {
      case '+': {
        if (stack.size() == 0) {
          dbg(original_sv, sv);
          throw err::ReErr(original_sv, "empty plus");
          sv.remove_prefix(1);
          break;
        }
        auto k = new Re(Plus());
        std::get<Plus>(*k).inner = std::move(stack.back());
        stack.back().reset(k);
        sv.remove_prefix(1);
        break;

      }
      case '*': {
        if (stack.size() == 0) {
          dbg(original_sv, sv);
          throw err::ReErr(original_sv, "empty kleene");
          sv.remove_prefix(1);
          break;
        }
        auto k = new Re(Kleene());
        std::get<Kleene>(*k).inner = std::move(stack.back());
        stack.back().reset(k);
        sv.remove_prefix(1);
        break;
      }
      case '?': {
        auto re = new Re(Disjunction());
        auto& inners = std::get<Disjunction>(*re).inners;
        inners.resize(2);
        inners.emplace_back(std::move(stack.back()));
        inners.emplace_back(std::make_unique<Re>(Eps()));
        stack.back().reset(re);
        sv.remove_prefix(1);
        break;
      }
      case '.': {
        auto re = std::make_unique<Re>(Disjunction());
        auto& inners = std::get<Disjunction>(*re).inners;
        for (int i = 0; i < 256; ++i) inners.push_back(std::make_unique<Re>(char(i)));
        sv.remove_prefix(1);
        break;
      }
      case '[': {
        int right_idx = 1;
        while (right_idx < sv.size() && sv[right_idx] != ']') right_idx++;
        if (right_idx == sv.size()) {
          dbg(original_sv, sv);
          throw err::ReErr(original_sv, "brackets not match, lack of right bracket");
          sv.remove_prefix(1);
          break;
        }
        if (right_idx == 1) {
          // empty
          sv.remove_prefix(2);
          break;
        }
        // [ sv[1]...sv[right_idx - 1] ]
        auto re = parse_brackets(sv.substr(1, right_idx - 1));
        stack.push_back(std::move(re));
        sv.remove_prefix(right_idx + 1);
        break;
      }
      case ']': {
        dbg(original_sv, sv);
        throw err::ReErr(original_sv, "brackets not match, too many right brackets");
        sv.remove_prefix(1);
        break;
      }
      case '(': {
        // brace matchable
        int right_idx = sv.size() - 1;
        while (right_idx >= 0 && sv[right_idx] != ')') right_idx--;
        if (right_idx < 0) {
          dbg(original_sv, sv);
          throw err::ReErr(original_sv, "brace not match, lack of right brace");
          // skip
          sv.remove_prefix(1);
          break;
        }

        if (right_idx == 1) {
          // empty
          sv.remove_prefix(2);
          break;
        }
        // ( sv[1]...sv[right_idx - 1] )
        auto re = parse_without_pipe(sv.substr(1, right_idx - 1));
        stack.push_back(std::move(re));
        sv.remove_prefix(right_idx + 1);
        break;
      }
      case ')': {
        dbg(original_sv, sv);
        throw err::ReErr(original_sv, "brace not match, too many right braces");
        sv.remove_prefix(1);
        break;
      }
      case '|': {
        UNREACHABLE();
      }
      default: {
        auto re = std::make_unique<Re>(char(sv[0]));
        stack.push_back(std::move(re));
        sv.remove_prefix(1);
        break;
      }
    }
  }

  auto res = new Re(Concat());
  auto& inners = std::get<Concat>(*res).inners;
  for (auto& re: stack) {
    inners.push_back(std::move(re));
  }

  return std::unique_ptr<Re>(res);
}


std::unique_ptr<Re> parse(std::string_view sv) {
  std::vector<std::string_view> output = split(sv, "|");
  if (output.size() == 1) {
    return parse_without_pipe(output[0]);
  }

  auto res = new Re(Disjunction());
  auto& inners = std::get<Disjunction>(*res).inners;
  inners.reserve(output.size());
  for (auto s: output) {
    assert(!s.empty());
    auto one = parse_without_pipe(s);
    inners.emplace_back(std::move(one));
  }
  return std::unique_ptr<Re>(res);
}


};  // namespace re2dfa::re
