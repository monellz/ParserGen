#pragma once

#include <memory>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "common.hpp"

namespace parsergen::re {

struct Re {
  bool nullable;
  std::unordered_set<int> firstpos;
  std::unordered_set<int> lastpos;

  virtual void traverse(std::unordered_map<int, u8>& leafpos_map,
                        std::unordered_map<int, std::unordered_set<int>>& followpos) = 0;
  virtual Re* clone() = 0;
  virtual ~Re(){};
};

struct Eps : Re {
  Eps() {
    nullable = true;
    firstpos = {};
    lastpos = {};
  }
  void traverse(std::unordered_map<int, u8>& leafpos_map, std::unordered_map<int, std::unordered_set<int>>& followpos) {
    // done in initialization
    /*
    nullable = true;
    firstpos = {};
    lastpos = {};
    */
  }

  Re* clone() { return new Eps(); }
};

struct Char : Re {
  char c;
  int leaf_count;
  Char(char _c, int _leaf_count) : c(_c), leaf_count(_leaf_count) {
    nullable = false;
    firstpos = {leaf_count};
    lastpos = {leaf_count};
  }
  void traverse(std::unordered_map<int, u8>& leafpos_map, std::unordered_map<int, std::unordered_set<int>>& followpos) {
    // partly done in initialization
    /*
    nullable = false;
    firstpos = {leaf_count};
    lastpos = {leaf_count};
    */

    leafpos_map[leaf_count] = c;
  }
  Re* clone() { return new Char(c, leaf_count); }
};

struct Kleene : Re {
  Re* son;
  Kleene(Re* _son) : son(_son) {}
  void traverse(std::unordered_map<int, u8>& leafpos_map, std::unordered_map<int, std::unordered_set<int>>& followpos) {
    son->traverse(leafpos_map, followpos);
    nullable = true;
    firstpos = son->firstpos;
    lastpos = son->lastpos;

    for (auto pos : lastpos) {
      union_inplace(followpos[pos], firstpos);
    }
  }
  Re* clone() { return new Kleene(son->clone()); }
  ~Kleene() {
    if (son) delete son;
  }
};

struct Concat : Re {
  std::vector<Re*> sons;
  Re* clone() {
    auto res = new Concat();
    for (auto son : sons) {
      res->sons.push_back(son->clone());
    }
    return res;
  }
  ~Concat() {
    for (auto son : sons) {
      if (son) delete son;
    }
    sons.clear();
  }
  void traverse(std::unordered_map<int, u8>& leafpos_map, std::unordered_map<int, std::unordered_set<int>>& followpos) {
    nullable = true;
    bool stop = false;
    for (auto son : sons) {
      son->traverse(leafpos_map, followpos);
      nullable = nullable && son->nullable;
      if (!stop) {
        union_inplace(firstpos, son->firstpos);
        if (!son->nullable) stop = true;
      }
    }

    stop = false;
    for (auto it = sons.rbegin(); it != sons.rend(); it++) {
      if (!stop) {
        union_inplace(lastpos, (*it)->lastpos);
        if (!(*it)->nullable) stop = true;
      }
    }

    // followpos
    std::unordered_set<int> tmp_lastpos;
    for (int i = 0; i < int(sons.size()) - 1; ++i) {
      if (sons[i]->nullable)
        union_inplace(tmp_lastpos, sons[i]->lastpos);
      else
        tmp_lastpos = sons[i]->lastpos;
      for (auto pos : tmp_lastpos) {
        union_inplace(followpos[pos], sons[i + 1]->firstpos);
      }
    }
  }
};

struct Disjunction : Re {
  std::vector<Re*> sons;
  Re* clone() {
    auto res = new Disjunction();
    for (auto son : sons) {
      res->sons.push_back(son->clone());
    }
    return res;
  }
  ~Disjunction() {
    for (auto son : sons) {
      if (son) delete son;
    }
    sons.clear();
  }
  void traverse(std::unordered_map<int, u8>& leafpos_map, std::unordered_map<int, std::unordered_set<int>>& followpos) {
    nullable = false;
    for (auto son : sons) {
      son->traverse(leafpos_map, followpos);
      nullable = nullable || son->nullable;
      union_inplace(firstpos, son->firstpos);
      union_inplace(lastpos, son->lastpos);
    }
  }
};

class ReEngine {
 private:
  void _parse_escaped_metachar(std::string_view sv, std::function<void(char)> update);

 public:
  ReEngine() : leaf_count(0) {}
  int leaf_count;
  static std::tuple<std::unique_ptr<Re>, int> produce(std::string_view sv) {
    ReEngine engine;
    return std::make_tuple(engine.parse(sv), engine.leaf_count);
  }
  std::unique_ptr<Re> parse(std::string_view sv);
  std::unique_ptr<Re> parse_without_pipe(std::string_view sv);
  std::unique_ptr<Re> parse_brackets(std::string_view sv);
};

void ReEngine::_parse_escaped_metachar(std::string_view sv, std::function<void(char)> update) {
  assert(sv[0] == '\\');
  assert(sv.size() == 2);
  switch (sv[1]) {
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
      update(' ');
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
      ERR_EXIT(sv, "unsupported char for escaping: " + std::string(sv.substr(0, 2)));
    }
  }
}

std::unique_ptr<Re> ReEngine::parse_brackets(std::string_view sv) {
  std::string_view original_sv = sv;

  std::unordered_set<char> hs;
  hs.reserve(256);

  std::function<void(char)> update;
  if (sv[0] == '^') {
    for (int i = 0; i < 256; ++i) hs.insert(i);
    sv.remove_prefix(1);
    update = [&](char c) { hs.erase(c); };
  } else {
    update = [&](char c) { hs.insert(c); };
  }

  std::vector<int> minus_idx;
  auto tmp_dis = std::make_unique<Disjunction>();
  while (!sv.empty()) {
    if (sv[0] == '\\') {
      if (sv.size() == 1) {
        ERR_EXIT(original_sv, "escape char is not complete");
      }
      std::unordered_set<char> tmp_hs;
      std::function<void(char)> tmp_update = [&](char c) { tmp_hs.insert(c); };
      _parse_escaped_metachar(sv.substr(0, 2), tmp_update);
      if (tmp_hs.size() > 1) {
        // use disjunction
        auto d = new Disjunction();
        for (auto c : tmp_hs) d->sons.push_back(new Char(c, -1));
        tmp_dis->sons.push_back(d);
      } else {
        assert(tmp_hs.size() == 1);
        auto c = new Char(*tmp_hs.begin(), -1);
        tmp_dis->sons.push_back(c);
      }
      sv.remove_prefix(2);
      continue;
    }

    switch (sv[0]) {
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
        ERR_EXIT(original_sv, sv[0], "not support unescaped metachar in brackets");
      }
      case '\\': {
        UNREACHABLE();
      }
      case '-': {
        minus_idx.push_back(tmp_dis->sons.size());
        sv.remove_prefix(1);
        break;
      }
      default: {
        auto c = new Char(sv[0], -1);
        tmp_dis->sons.push_back(c);
        sv.remove_prefix(1);
        break;
      }
    }
  }

  // check minus ilegal
  for (auto idx : minus_idx) {
    // check [idx - 1] and [idx] exsistence
    if (idx < 1 || size_t(idx) >= tmp_dis->sons.size()) {
      update('-');
      continue;
    }

    // check continuity
    auto left = tmp_dis->sons[idx - 1];
    auto right = tmp_dis->sons[idx];
    if (dynamic_cast<Disjunction*>(left) || dynamic_cast<Disjunction*>(right)) {
      update('-');
      continue;
    }

    auto left_c = dynamic_cast<Char*>(left)->c;
    auto right_c = dynamic_cast<Char*>(right)->c;
    if (left_c > right_c) {
      update('-');
      continue;
    }

    // pass
    for (int i = left_c; i <= right_c; ++i) {
      update(i);
    }

    // manually delete for future check
    delete tmp_dis->sons[idx - 1];
    delete tmp_dis->sons[idx];
    tmp_dis->sons[idx - 1] = nullptr;
    tmp_dis->sons[idx] = nullptr;
  }

  for (auto& son : tmp_dis->sons) {
    if (son == nullptr) continue;
    if (auto dis = dynamic_cast<Disjunction*>(son)) {
      for (auto s : dis->sons) {
        assert(dynamic_cast<Char*>(s) != nullptr);
        update(dynamic_cast<Char*>(s)->c);
      }
    } else {
      assert(dynamic_cast<Char*>(son) != nullptr);
      update(dynamic_cast<Char*>(son)->c);
    }
  }

  // delete(and his sons)
  tmp_dis.reset();

  auto dis = std::make_unique<Disjunction>();
  for (auto c : hs) {
    dis->sons.push_back(new Char(c, leaf_count++));
  }

  return dis;
}

std::unique_ptr<Re> ReEngine::parse_without_pipe(std::string_view sv) {
  // meta char
  // ()[].|*+\? not support {} ^ $

  // stack elem use Concat
  auto concat = std::make_unique<Concat>();
  auto& stack = concat->sons;
  std::string_view original_sv = sv;

  std::function<void(char)> update = [&](char c) { stack.push_back(new Char(c, leaf_count++)); };

  while (!sv.empty()) {
    if (sv[0] == '\\') {
      if (sv.size() == 1) {
        ERR_EXIT(original_sv, "escaped char is not complete");
      }
      _parse_escaped_metachar(sv.substr(0, 2), update);
      sv.remove_prefix(2);
      continue;
    }

#define CHECK_STACK_FOR_UNARY_OP(err_msg)                  \
  do {                                                     \
    if (stack.size() == 0) ERR_EXIT(original_sv, err_msg); \
  } while (false)

    switch (sv[0]) {
      case '+': {
        CHECK_STACK_FOR_UNARY_OP("empty plus");
        auto k = new Kleene(stack.back()->clone());
        stack.push_back(k);
        sv.remove_prefix(1);
        break;
      }
      case '*': {
        CHECK_STACK_FOR_UNARY_OP("empty kleene");
        auto k = new Kleene(stack.back());
        stack.back() = k;
        sv.remove_prefix(1);
        break;
      }
      case '?': {
        CHECK_STACK_FOR_UNARY_OP("empty question mark");
        auto d = new Disjunction();
        d->sons.push_back(stack.back());
        d->sons.push_back(new Eps());
        stack.back() = d;
        sv.remove_prefix(1);
        break;
      }
      case '.': {
        auto d = new Disjunction();
        for (int i = 0; i < 256; ++i) d->sons.push_back(new Char(i, leaf_count++));
        sv.remove_prefix(1);
        break;
      }
      case '[': {
        // check close
        size_t right_idx = 1;
        while (right_idx < sv.size() && sv[right_idx] != ']') right_idx++;
        if (right_idx == sv.size()) ERR_EXIT(original_sv, sv, "brackets not match, lack of right bracket");
        if (right_idx == 1) {
          // empty bracket
          sv.remove_prefix(2);
          break;
        }

        // [ sv[1]...sv[right_idx - 1] ]
        auto b = parse_brackets(sv.substr(1, right_idx - 1));
        stack.push_back(b.release());
        sv.remove_prefix(right_idx + 1);
        break;
      }
      case '(': {
        // check close
        size_t right_idx = 1;
        while (right_idx < sv.size() && sv[right_idx] != ')') right_idx++;
        if (right_idx == sv.size()) ERR_EXIT(original_sv, sv, "brace not match, lack of right brace");
        if (right_idx == 1) {
          // empty brace
          sv.remove_prefix(2);
          break;
        }

        // ( sv[1]...sv[right_idx - 1] )
        auto b = parse_without_pipe(sv.substr(1, right_idx - 1));
        stack.push_back(b.release());
        sv.remove_prefix(1);
        break;
      }
      case ']': {
        ERR_EXIT(original_sv, sv, "brackets not match, too many right bracket");
      }
      case ')': {
        ERR_EXIT(original_sv, sv, "brace not match, too many right brace");
      }
      case '\\':
      case '|': {
        UNREACHABLE();
      }
      default: {
        stack.push_back(new Char(sv[0], leaf_count++));
        sv.remove_prefix(1);
        break;
      }
    }
  }

  return concat;
}

std::unique_ptr<Re> ReEngine::parse(std::string_view sv) {
  std::vector<std::string_view> output = split(sv, "|");
  if (output.size() == 1) return parse_without_pipe(output[0]);

  // auto dis = new Disjunction();
  auto dis = std::make_unique<Disjunction>();
  dis->sons.reserve(output.size());
  for (auto s : output) {
    assert(!s.empty());
    auto one = parse_without_pipe(s);
    dis->sons.emplace_back(one.release());
  }

  return dis;
}

}  // namespace parsergen::re
