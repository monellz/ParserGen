#ifndef __CORE_H
#define __CORE_H

#include <memory>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "core/common.h"
#include "util/casting.h"

namespace parsergen::re {

using intset_t = std::unordered_set<int>;

class Eps;
class Char;
class Kleene;
class Concat;
class Disjunction;

class Re {
 public:
  enum ReKind {
    kEps,
    kChar,
    kKleene,
    kConcat,
    kDisjunction,
  };

  ReKind kind;
  bool nullable;
  intset_t firstpos;
  intset_t lastpos;
  explicit Re(ReKind kind) : kind(kind) {}
  std::unique_ptr<Re> clone() const;
  virtual ~Re() {}
};

class Eps : public Re {
 public:
  explicit Eps() : Re(ReKind::kEps) {}
  static bool classof(const Re* base) { return base->kind == ReKind::kEps; }
  std::unique_ptr<Eps> clone_impl() const { return std::make_unique<Eps>(); }
  virtual ~Eps() override {}
};

class Char : public Re {
 public:
  char c;
  int leaf_idx;
  explicit Char(char c) : Re(ReKind::kChar), c(c) {}
  static bool classof(const Re* base) { return base->kind == ReKind::kChar; }
  std::unique_ptr<Char> clone_impl() const { return std::make_unique<Char>(c); }
  virtual ~Char() override {}
};

class Kleene : public Re {
 public:
  std::unique_ptr<Re> son;
  explicit Kleene(std::unique_ptr<Re> son)
      : Re(ReKind::kKleene), son(std::move(son)) {}
  static bool classof(const Re* base) { return base->kind == ReKind::kKleene; }

  std::unique_ptr<Kleene> clone_impl() const {
    auto son_cloned = son->clone();
    return std::make_unique<Kleene>(std::move(son_cloned));
  }
  virtual ~Kleene() override {}
};

class Concat : public Re {
 public:
  std::vector<std::unique_ptr<Re>> sons;
  explicit Concat() : Re(ReKind::kConcat) {}
  static bool classof(const Re* base) { return base->kind == ReKind::kConcat; }

  std::unique_ptr<Concat> clone_impl() const {
    auto new_concat = std::make_unique<Concat>();
    for (auto& son : sons) {
      auto son_cloned = son->clone();
      new_concat->sons.push_back(std::move(son_cloned));
    }
    return new_concat;
  }
  virtual ~Concat() override {}
};

class Disjunction : public Re {
 public:
  std::vector<std::unique_ptr<Re>> sons;
  explicit Disjunction() : Re(ReKind::kDisjunction) {}
  static bool classof(const Re* base) {
    return base->kind == ReKind::kDisjunction;
  }
  std::unique_ptr<Disjunction> clone_impl() const {
    auto new_dis = std::make_unique<Disjunction>();
    for (auto& son : sons) {
      auto son_cloned = son->clone();
      new_dis->sons.push_back(std::move(son_cloned));
    }
    return new_dis;
  }
  virtual ~Disjunction() override {}
};

class ReEngine {
 public:
  ReEngine() {}
  static std::unique_ptr<Re> produce(std::string_view sv) {
    ReEngine engine;
    return engine.parse(sv);
  }

  std::unique_ptr<Re> parse(std::string_view sv);
  std::unique_ptr<Re> parse_without_pipe(std::string_view sv);
  std::unique_ptr<Re> parse_brackets(std::string_view sv);

  std::unordered_set<char> _expand_metachar(std::string_view sv);
};

void dfs(std::unique_ptr<Re>& re,
         std::function<void(std::unique_ptr<re::Re>&)> fn);

}  // namespace parsergen::re

#endif
