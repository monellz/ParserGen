#pragma once

#include <vector>
#include "re.hpp"
#include "common.hpp"

namespace parsergen::dfa {

using DfaNode = std::unordered_map<u8, u32>;

struct Dfa {
  std::vector<std::tuple<DfaNode, bool>> nodes;
  Dfa(){}
  Dfa(const Dfa& d): nodes(d.nodes) {}
  Dfa(Dfa&& d): nodes(std::move(d.nodes)) {}

  bool accept(std::string_view sv) {
    assert(nodes.size() >= 1);
    u32 cur_idx = 0;
    bool cur_terminal = std::get<1>(nodes[0]);
    for (auto c: sv) {
      auto& [cur_node, _] = nodes[cur_idx];
      if (auto it = cur_node.find(c); it != cur_node.end()) {
        // found corresponding edge
        cur_idx = it->second;
        cur_terminal = std::get<1>(nodes[cur_idx]);
      } else {
        return false;
      }
    }
    return cur_terminal;
  }
};

class DfaEngine {
private:
  std::shared_ptr<re::Re> re;
  std::shared_ptr<re::Re> expand_re;

  std::vector<std::unordered_set<int>> states;

  std::unordered_map<int, u8> leafpos_map;
  int leaf_count;

  const char TERMINATION = '\0';
  int TERMINATION_INDEX;

  std::unordered_map<std::shared_ptr<re::Re>, bool> nullable;
  std::unordered_map<std::shared_ptr<re::Re>, std::unordered_set<int>> firstpos;
  std::unordered_map<std::shared_ptr<re::Re>, std::unordered_set<int>> lastpos;
  std::unordered_map<int, std::unordered_set<int>> followpos;
  
  void _traverse(const std::shared_ptr<re::Re>& cur) {
    std::visit(overloaded {
      [&] (const char c) {
        nullable[cur] = false;
        firstpos[cur] = {leaf_count};
        lastpos[cur] = {leaf_count};

        leafpos_map[leaf_count++] = c;
      },
      [&] (const re::Eps&) {
        nullable[cur] = true;
        firstpos[cur] = {};
        lastpos[cur] = {};
      },
      [&] (const re::Kleene& k) {
        auto son = k.inner;
        _traverse(son);
        
        nullable[cur] = true;
        firstpos[cur] = firstpos[son];
        lastpos[cur] = lastpos[son];

        for (auto pos: lastpos[cur]) {
          union_inplace(followpos[pos], firstpos[cur]);
        }
      },
      [&] (const re::Concat& c) {
        nullable[cur] = true;
        bool stop = false;
        for (auto son: c.inners) {
          _traverse(son);
          nullable[cur] = nullable[cur] && nullable[son];
          if (!stop) {
            union_inplace(firstpos[cur], firstpos[son]);
            if (!nullable[son]) stop = true;
          }
        }
        stop = false;
        for (auto it = c.inners.rbegin(); it != c.inners.rend(); it++) {
          if (!stop) {
            union_inplace(lastpos[cur], lastpos[*it]);
            if (!nullable[*it]) stop = true;
          }
        }

        // followpos
        for (int i = 0; i < c.inners.size() - 1; i++) {
          for (auto pos: lastpos[c.inners[i]]) {
            union_inplace(followpos[pos], firstpos[c.inners[i + 1]]);
          }
        }
      },
      [&] (const re::Disjunction& d) {
        nullable[cur] = false;
        for (auto son: d.inners) {
          _traverse(son);
          nullable[cur] = nullable[cur] || nullable[son];
          union_inplace(firstpos[cur], firstpos[son]);
          union_inplace(lastpos[cur], lastpos[son]);
        }
      },
    }, *cur);
  }
public:
  DfaEngine(std::shared_ptr<re::Re>& _re): re(std::move(_re)), leaf_count(0), TERMINATION_INDEX(-1) {
    // use \0 as the termination
    expand_re = std::make_shared<re::Re>(re::Concat());

    std::get<re::Concat>(*expand_re).inners.push_back(re); // !!! use INTERNAL member instead of args (it's MOVED!!!)
    std::get<re::Concat>(*expand_re).inners.push_back(std::make_shared<re::Re>(char(TERMINATION)));

    _traverse(expand_re);

    TERMINATION_INDEX = leaf_count - 1; 
    assert(leafpos_map.at(TERMINATION_INDEX) == TERMINATION);
  }

  Dfa produce() {
    states.clear();
    Dfa dfa;

    int label_idx = 0;
    states.push_back(firstpos[expand_re]);
    auto get_state_idx = [this](std::unordered_set<int>& s) -> int {
      for (int i = 0; i < this->states.size(); ++i) {
        if (this->states[i] == s) return i;
      }
      this->states.push_back(std::move(s));
      return int(this->states.size() - 1);
    };
    while (label_idx < states.size()) {
      const auto& s = states[label_idx];
      bool is_terminal = (s.find(TERMINATION_INDEX) != s.end());
      dfa.nodes.push_back(std::make_tuple(DfaNode(), is_terminal));
      auto& [cur, _] = dfa.nodes[label_idx];
      std::unordered_map<u8, std::unordered_set<int>> u;
      for (auto pos: s) u[leafpos_map.at(pos)].insert(pos);
      for (auto& [c, pos_set]: u) {
        std::unordered_set<int> new_set;
        // followpos use [] instead of AT, because default followpos is empty set
        for (auto pos: pos_set) union_inplace(new_set, followpos[pos]);
        auto next_state_idx = get_state_idx(new_set);
        cur[c] = next_state_idx;
      }
      label_idx++;
    }

    return dfa;
  }

  static std::tuple<DfaEngine, Dfa> produce(std::string_view sv) {
    auto re = re::parse(sv);
    DfaEngine engine(re);
    auto dfa = engine.produce();
    return std::make_tuple(engine, dfa);
  }
};
  
} // namespace parsergen::dfa