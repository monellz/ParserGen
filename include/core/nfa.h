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

  // "Compilers: Principles, Techniques and Tools" Algorithm 3.23
  static Nfa from_re(std::unique_ptr<re::Re>& re, u32 id = 0) {
    // TODO: more efficiency
    std::unordered_map<re::Re*, std::vector<NfaNode>> dfa_sons;
    re::dfs(re, [&](std::unique_ptr<re::Re>& _re) {
      std::vector<NfaNode> nodes;
      switch (_re->kind) {
        case re::Re::kEps: {
          nodes.emplace_back(std::nullopt, std::vector<u32>{1},
                             std::unordered_map<u8, std::vector<u32>>());
          nodes.emplace_back(std::make_optional<u32>(id), std::vector<u32>(),
                             std::unordered_map<u8, std::vector<u32>>());
          break;
        }
        case re::Re::kChar: {
          auto char_re = static_cast<re::Char*>(_re.get());
          nodes.emplace_back(std::nullopt, std::vector<u32>(),
                             std::unordered_map<u8, std::vector<u32>>{
                                 {char_re->c, std::vector<u32>{1}}});
          nodes.emplace_back(std::make_optional<u32>(id), std::vector<u32>(),
                             std::unordered_map<u8, std::vector<u32>>());
          break;
        }
        case re::Re::kKleene: {
          auto k_re = static_cast<re::Kleene*>(_re.get());
          auto son = std::move(dfa_sons[k_re->son.get()]);
          dfa_sons.erase(k_re->son.get());
          u32 son_node_size = son.size();

          // start node
          // nodes[son_it.second.size() + 1] should be the final end node
          nodes.emplace_back(std::nullopt,
                             std::vector<u32>{1, son_node_size + 1},
                             std::unordered_map<u8, std::vector<u32>>());
          for (auto& son_node : son) son_node.move_offset(1);
          for (u32 i = 0; i < son_node_size; ++i)
            nodes.push_back(std::move(son[i]));
          // TODO: maybe we should use bitset
          bool found = false;
          for (auto i : nodes.back().eps_edges) {
            if (i == 1) {
              found = true;
              break;
            }
          }
          if (!found) nodes.back().eps_edges.push_back(1);
          nodes.back().eps_edges.push_back(son_node_size + 1);
          nodes.back().terminal_id = std::nullopt;

          // end node
          nodes.emplace_back(std::make_optional<u32>(id), std::vector<u32>(),
                             std::unordered_map<u8, std::vector<u32>>());
          assert(nodes.size() == son_node_size + 2);
          break;
        }
        case re::Re::kConcat: {
          auto con_re = static_cast<re::Concat*>(_re.get());
          auto first_son = std::move(dfa_sons[con_re->sons[0].get()]);
          u32 first_son_node_size = first_son.size();
          dfa_sons.erase(con_re->sons[0].get());
          u32 start_node_idx = first_son.size() - 1;
          for (u32 j = 0; j < first_son_node_size; ++j)
            nodes.push_back(std::move(first_son[j]));

          for (u32 i = 1; i < (u32)con_re->sons.size(); ++i) {
            assert(nodes[start_node_idx].terminal_id);
            nodes[start_node_idx].terminal_id = std::nullopt;

            auto son = dfa_sons[con_re->sons[i].get()];
            u32 son_node_size = son.size();
            dfa_sons.erase(con_re->sons[i].get());
            // move edges to the end node of previous son
            // fix index in son nodes
            for (auto& son_node : son) son_node.move_offset(start_node_idx);
            nodes[start_node_idx].eps_edges = std::move(son[0].eps_edges);
            nodes[start_node_idx].edges = std::move(son[0].edges);
            // push other nodes of son
            for (u32 j = 1; j < son_node_size; ++j)
              nodes.push_back(std::move(son[j]));
            assert(nodes.back().terminal_id);
            nodes.back().terminal_id = std::nullopt;

            start_node_idx += son_node_size - 1;
          }
          assert(nodes.size() - 1 == start_node_idx);
          nodes.back().terminal_id = id;
          break;
        }
        case re::Re::kDisjunction: {
          auto dis_re = static_cast<re::Disjunction*>(_re.get());
          nodes.emplace_back(std::nullopt, std::vector<u32>(),
                             std::unordered_map<u8, std::vector<u32>>());

          u32 start_offset = 1;
          std::vector<u32> start_eps_edges;
          std::vector<u32> sons_end_nodes;
          for (u32 i = 0; i < dis_re->sons.size(); ++i) {
            auto son = dfa_sons[dis_re->sons[i].get()];
            u32 son_node_size = son.size();
            dfa_sons.erase(dis_re->sons[i].get());
            nodes[0].eps_edges.push_back(start_offset);
            // fix index in son nodes
            for (auto& son_node : son) son_node.move_offset(start_offset);
            // push son's nodes
            for (u32 j = 0; j < son_node_size; ++j)
              nodes.push_back(std::move(son[j]));
            nodes.back().terminal_id = std::nullopt;

            start_offset += son_node_size;
            sons_end_nodes.push_back(start_offset - 1);
            assert(nodes.size() == start_offset);
          }

          // for every end node of son, make it point the final end node (eps
          // edge)
          for (auto son_end_node_idx : sons_end_nodes) {
            nodes[son_end_node_idx].eps_edges.push_back(start_offset);
          }

          // end node
          nodes.emplace_back(std::make_optional<u32>(id), std::vector<u32>(),
                             std::unordered_map<u8, std::vector<u32>>());
          break;
        }
      }
      dfa_sons.emplace(_re.get(), std::move(nodes));
    });
    assert(dfa_sons.size() == 1);
    return Nfa(std::move(dfa_sons[re.get()]));
  }
};

}  // namespace parsergen::nfa

#endif
