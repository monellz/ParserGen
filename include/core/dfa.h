#ifndef __DFA_H
#define __DFA_H

#include <optional>
#include <utility>
#include <vector>

#include "core/common.h"
#include "core/dfa.h"
#include "core/re.h"

namespace parsergen::dfa {

using DfaNode = std::pair<std::optional<u32>, std::unordered_map<u8, u32>>;

struct Dfa {
  std::vector<DfaNode> nodes;
  Dfa() {}
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
};

class DfaEngine {
 private:
  u32 id;
  std::unique_ptr<re::Re> expand_re;

  std::vector<std::unordered_set<int>> states;

  std::unordered_map<int, u8> leafpos_map;
  int leaf_count;

  const char TERMINATION = '\0';
  int TERMINATION_INDEX;

  std::unordered_map<int, std::unordered_set<int>> followpos;

 public:
  void dfs(std::unique_ptr<re::Re>& re,
           std::function<void(std::unique_ptr<re::Re>&)> fn) {
    switch (re->kind) {
      case re::Re::kChar:
      case re::Re::kEps:
        break;
      case re::Re::kKleene: {
        auto s = static_cast<re::Kleene*>(re.get());
        dfs(s->son, fn);
        break;
      }
      case re::Re::kConcat: {
        auto c = static_cast<re::Concat*>(re.get());
        for (auto& s : c->sons) {
          dfs(s, fn);
        }
        break;
      }
      case re::Re::kDisjunction: {
        auto dis = static_cast<re::Disjunction*>(re.get());
        for (auto& s : dis->sons) {
          dfs(s, fn);
        }
        break;
      }
      default:
        UNREACHABLE();
    }
    fn(re);
  }
  DfaEngine(std::unique_ptr<re::Re> re, u32 id = 0) : id(id) {
    auto ex_re = std::make_unique<re::Concat>();
    ex_re->sons.push_back(std::move(re));
    ex_re->sons.push_back(std::make_unique<re::Char>('\0'));

    expand_re = std::move(ex_re);

    leaf_count = 0;
    dfs(expand_re, [&](std::unique_ptr<re::Re>& _re) {
      // switch
      // leafpo
      switch (_re->kind) {
        case re::Re::kChar: {
          auto char_re = static_cast<re::Char*>(_re.get());
          char_re->leaf_idx = leaf_count;
          char_re->nullable = false;
          char_re->firstpos = {leaf_count};
          char_re->lastpos = {leaf_count};
          leafpos_map[leaf_count] = char_re->c;
          leaf_count++;
          break;
        }
        case re::Re::kEps: {
          auto eps_re = static_cast<re::Eps*>(_re.get());
          eps_re->nullable = true;
          break;
        }
        case re::Re::kKleene: {
          auto k_re = static_cast<re::Kleene*>(_re.get());
          k_re->nullable = true;
          k_re->firstpos = k_re->son->firstpos;
          k_re->lastpos = k_re->son->lastpos;

          for (auto pos : k_re->lastpos) {
            union_inplace(this->followpos[pos], k_re->firstpos);
          }
          break;
        }
        case re::Re::kConcat: {
          auto con_re = static_cast<re::Concat*>(_re.get());
          con_re->nullable = true;
          bool stop = false;
          for (auto& son : con_re->sons) {
            con_re->nullable = con_re->nullable && son->nullable;
            if (!stop) {
              union_inplace(con_re->firstpos, son->firstpos);
              if (!son->nullable) stop = true;
            }
          }

          stop = false;
          for (auto it = con_re->sons.rbegin(); it != con_re->sons.rend();
               ++it) {
            if (!stop) {
              union_inplace(con_re->lastpos, (*it)->lastpos);
              if (!(*it)->nullable) stop = true;
            }
          }

          // followpos
          std::unordered_set<int> tmp_lastpos;
          for (int i = 0; i < (int)con_re->sons.size() - 1; ++i) {
            if (con_re->sons[i]->nullable) {
              union_inplace(tmp_lastpos, con_re->sons[i]->lastpos);
            } else {
              tmp_lastpos = con_re->sons[i]->lastpos;
            }
            for (auto pos : tmp_lastpos) {
              union_inplace(followpos[pos], con_re->sons[i + 1]->firstpos);
            }
          }
          break;
        }
        case re::Re::kDisjunction: {
          auto dis_re = static_cast<re::Disjunction*>(_re.get());
          for (auto& son : dis_re->sons) {
            dis_re->nullable = dis_re->nullable || son->nullable;
            union_inplace(dis_re->firstpos, son->firstpos);
            union_inplace(dis_re->lastpos, son->lastpos);
          }
          break;
        }
        default:
          UNREACHABLE();
      }
    });

    TERMINATION_INDEX = leaf_count - 1;
    assert(leafpos_map.at(TERMINATION_INDEX) == TERMINATION);
  }

  Dfa produce() {
    states.clear();
    Dfa dfa;

    size_t label_idx = 0;
    states.push_back(expand_re->firstpos);
    auto get_state_idx = [this](std::unordered_set<int>& s) -> int {
      for (size_t i = 0; i < this->states.size(); ++i) {
        if (this->states[i] == s) return i;
      }
      this->states.push_back(std::move(s));
      return int(this->states.size() - 1);
    };
    while (label_idx < states.size()) {
      const auto& s = states[label_idx];
      std::optional<u32> is_terminal;
      if (s.find(TERMINATION_INDEX) != s.end()) {
        is_terminal = this->id;
      }
      DfaNode tmp_node;
      std::get<0>(tmp_node) = is_terminal;
      dfa.nodes.push_back(std::move(tmp_node));
      auto& [_, cur] = dfa.nodes[label_idx];
      std::unordered_map<u8, std::unordered_set<int>> u;
      for (auto pos : s) u[leafpos_map.at(pos)].insert(pos);
      for (auto& [c, pos_set] : u) {
        std::unordered_set<int> new_set;
        // followpos use [] instead of AT, because default followpos is empty
        // set
        for (auto pos : pos_set) union_inplace(new_set, followpos[pos]);
        auto next_state_idx = get_state_idx(new_set);
        cur[c] = next_state_idx;
      }
      label_idx++;
    }

    // clean TERMINATION
    for (auto& [_, node] : dfa.nodes) {
      node.erase(TERMINATION);
    }

    return dfa;
  }

  static std::tuple<DfaEngine, Dfa> produce(std::string_view sv, u32 id = 0) {
    auto re = re::ReEngine::produce(sv);
    DfaEngine engine(std::move(re), id);
    auto dfa = engine.produce();
    dfa.minimize();
    return std::make_tuple(std::move(engine), std::move(dfa));
  }
};

}  // namespace parsergen::dfa

#endif
