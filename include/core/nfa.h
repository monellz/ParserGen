#ifndef __NFA_H
#define __NFA_H

#include <optional>
#include <unordered_map>
#include <vector>

#include "core/common.h"
#include "core/re.h"

namespace parsergen::nfa {

struct NfaNode {
  std::optional<u32> terminal_id;
  std::vector<u32> eps_edges;
  std::unordered_map<u8, std::vector<u32>> edges;

  NfaNode(std::optional<u32> terminal_id, std::vector<u32> eps_edges,
          std::unordered_map<u8, std::vector<u32>> edges)
      : terminal_id(std::move(terminal_id)),
        eps_edges(std::move(eps_edges)),
        edges(std::move(edges)) {}

  void move_offset(u32 offset) {
    for (auto& e : eps_edges) e += offset;
    for (auto& [k, v] : edges) {
      for (auto& e : v) e += offset;
    }
  }
};

struct Nfa {
  // nodes[0] is the start
  std::vector<NfaNode> nodes;
  explicit Nfa(std::vector<NfaNode> nodes) : nodes(std::move(nodes)) {}
  Nfa(const Nfa& d) : nodes(d.nodes) {}
  Nfa(Nfa&& d) : nodes(std::move(d.nodes)) {}

  static Nfa from_re(std::unique_ptr<re::Re>& re, u32 id = 0);
  static Nfa from_re(std::vector<std::unique_ptr<re::Re>>& res);
};

}  // namespace parsergen::nfa

#endif
