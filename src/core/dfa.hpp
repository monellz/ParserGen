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
      const auto& [cur_node, _] = nodes[cur_idx];
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

  void minimize() {
    // now only remove dead state
    // TODO: more efficient
    std::vector<bool> visit(nodes.size(), false);
    std::vector<int> stack;
    stack.reserve(nodes.size());

    stack.push_back(0);
    visit[0] = true;
    int alive_num = 0;
    std::unordered_map<int, int> reindex;
    while (!stack.empty()) {
      int cur = stack.back();
      stack.pop_back();
      reindex[cur] = alive_num++;
      const auto& [node, _] = nodes[cur];
      for (const auto& [__, next]: node) {
        if (!visit[next]) {
          visit[next] = true;
          stack.push_back(next);
        }
      }
    }

    std::vector<std::tuple<DfaNode, bool>> new_nodes(alive_num);

    for (int i = 0; i < visit.size(); ++i) {
      if (visit[i]) {
        // alive node

        new_nodes[reindex.at(i)] = std::move(nodes[i]);
        auto& [node, _] = new_nodes[reindex.at(i)];
        for (auto it = node.begin(); it != node.end(); ++it) {
          it->second = reindex.at(it->second);
        }
      }
    }

    nodes = std::move(new_nodes);
  }
};

class DfaEngine {
private:
  std::unique_ptr<re::Re> expand_re;

  std::vector<std::unordered_set<int>> states;

  std::unordered_map<int, u8> leafpos_map;
  int leaf_count;

  const char TERMINATION = '\0';
  int TERMINATION_INDEX;

  std::unordered_map<int, std::unordered_set<int>> followpos;
public:
  DfaEngine(std::unique_ptr<re::Re> _re, int _leaf_count): leaf_count(_leaf_count) {
    auto ex_re = new re::Concat();
    ex_re->sons.push_back(_re.release());
    ex_re->sons.push_back(new re::Char('\0', leaf_count++));
    ex_re->traverse(leafpos_map, followpos);

    expand_re.reset(ex_re);

    TERMINATION_INDEX = leaf_count - 1;
    assert(leafpos_map.at(TERMINATION_INDEX) == TERMINATION);
  }

  Dfa produce() {
    states.clear();
    Dfa dfa;

    int label_idx = 0;
    states.push_back(expand_re->firstpos);
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

    // clean TERMINATION
    for (auto& [node, _]: dfa.nodes) {
      node.erase(TERMINATION);
    }

    return dfa;
  }

  static std::tuple<DfaEngine, Dfa> produce(std::string_view sv) {
    auto [re, leaf_count] = re::ReEngine::produce(sv);
    DfaEngine engine(std::move(re), leaf_count);
    auto dfa = engine.produce();
    dfa.minimize();
    return std::make_tuple(std::move(engine), std::move(dfa));
  }
};
  
} // namespace parsergen::dfa