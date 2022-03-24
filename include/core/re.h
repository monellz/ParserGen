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
  Re* clone() const;
};

class Eps : public Re {
 public:
  explicit Eps() : Re(ReKind::kEps) {}
  static bool classof(const Re* base) { return base->kind == ReKind::kEps; }
  Eps* clone_impl() const { return new Eps(); }
};

class Char : public Re {
 public:
  char c;
  int leaf_idx;
  explicit Char(char c) : Re(ReKind::kChar), c(c) {}
  static bool classof(const Re* base) { return base->kind == ReKind::kChar; }
  Char* clone_impl() const { return new Char(c); }
};

class Kleene : public Re {
 public:
  std::unique_ptr<Re> son;
  explicit Kleene(Re* son) : Re(ReKind::kKleene), son(son) {}
  static bool classof(const Re* base) { return base->kind == ReKind::kKleene; }

  Kleene* clone_impl() const { return new Kleene(son->clone()); }
};

class Concat : public Re {
 public:
  std::vector<std::unique_ptr<Re>> sons;
  explicit Concat() : Re(ReKind::kConcat) {}
  static bool classof(const Re* base) { return base->kind == ReKind::kConcat; }

  inline void append(Re* _re) { sons.push_back(std::unique_ptr<Re>(_re)); }

  Concat* clone_impl() const {
    Concat* new_concat = new Concat();
    for (auto& son : sons) {
      new_concat->append(son->clone());
    }
    return new_concat;
  }
};

class Disjunction : public Re {
 public:
  std::vector<std::unique_ptr<Re>> sons;
  explicit Disjunction() : Re(ReKind::kDisjunction) {}
  static bool classof(const Re* base) {
    return base->kind == ReKind::kDisjunction;
  }
  inline void append(Re* _re) { sons.push_back(std::unique_ptr<Re>(_re)); }
  Disjunction* clone_impl() const {
    Disjunction* new_dis = new Disjunction();
    for (auto& son : sons) {
      new_dis->append(son->clone());
    }
    return new_dis;
  }
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

}  // namespace parsergen::re

#endif
