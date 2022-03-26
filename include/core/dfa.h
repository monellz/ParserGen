#ifndef __DFA_H
#define __DFA_H

#include <bitset>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "core/common.h"
#include "core/nfa.h"
#include "core/re.h"

namespace parsergen::dfa {

using DfaNode = std::pair<std::optional<u32>, std::unordered_map<u8, u32>>;

struct Dfa {
  std::vector<DfaNode> nodes;
  explicit Dfa(std::vector<DfaNode>&& nodes) : nodes(std::move(nodes)) {}
  Dfa(const Dfa& d) : nodes(d.nodes) {}
  Dfa(Dfa&& d) : nodes(std::move(d.nodes)) {}

  // if ret_val.has_value() / if (ret_val) it is accepted
  std::optional<u32> accept(std::string_view sv) {
    assert(nodes.size() >= 1);
    u32 cur_idx = 0;
    auto terminal = std::get<0>(nodes[0]);
    for (auto c : sv) {
      const auto& cur_node = nodes[cur_idx];
      const auto& edges = std::get<1>(cur_node);
      if (auto it = edges.find(c); it != edges.end()) {
        // found corresponding edge
        cur_idx = it->second;
        terminal = std::get<0>(nodes[cur_idx]);
      } else {
        return std::nullopt;
      }
    }
    return terminal;
  }

  void remove_dead_state();
  void minimize();

  static Dfa from_sv(std::string_view sv, u32 id = 0);
  static Dfa from_re(std::unique_ptr<re::Re>&& re, u32 id = 0);
  static Dfa from_nfa(nfa::Nfa&& nfa);
};

void bfs(Dfa& dfa, std::function<void(u32, DfaNode&)> fn);
void dfs(Dfa& dfa, std::function<void(u32, DfaNode&)> fn);

}  // namespace parsergen::dfa

#endif
