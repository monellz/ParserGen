#ifndef __DFA_H
#define __DFA_H

#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "core/common.h"
#include "core/re.h"

namespace parsergen::dfa {

using DfaNode = std::pair<std::optional<u32>, std::unordered_map<u8, u32>>;

struct Dfa {
  std::vector<DfaNode> nodes;
  explicit Dfa(std::vector<DfaNode> nodes) : nodes(std::move(nodes)) {}
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

  void minimize() {
    // now only remove dead state
    // TODO: more efficient
    /*
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
      for (const auto& [__, next] : node) {
        if (!visit[next]) {
          visit[next] = true;
          stack.push_back(next);
        }
      }
    }

    std::vector<DfaNode> new_nodes(alive_num);

    for (size_t i = 0; i < visit.size(); ++i) {
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
    */
  }

  static Dfa from_re(std::unique_ptr<re::Re>&& re, u32 id = 0);
  static Dfa from_sv(std::string_view sv, u32 id = 0);
};

}  // namespace parsergen::dfa

#endif
