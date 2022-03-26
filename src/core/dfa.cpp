#include "core/dfa.h"

#include <vector>

#include "core/common.h"
#include "core/re.h"

namespace parsergen::dfa {

Dfa Dfa::from_re(std::unique_ptr<re::Re>&& re, u32 id) {
  const char TERMINATION = '\0';
  int TERMINATION_INDEX;
  std::unordered_map<int, u8> leafpos_map;
  std::unordered_map<int, std::unordered_set<int>> followpos;

  auto new_re = std::make_unique<re::Concat>();
  new_re->sons.push_back(std::move(re));
  new_re->sons.push_back(std::make_unique<re::Char>('\0'));
  std::unique_ptr<re::Re> ex_re = std::move(new_re);

  int leaf_count = 0;
  re::dfs(ex_re, [&](std::unique_ptr<re::Re>& _re) {
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
          union_inplace(followpos[pos], k_re->firstpos);
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
        for (auto it = con_re->sons.rbegin(); it != con_re->sons.rend(); ++it) {
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

  // produce
  std::vector<std::unordered_set<int>> states;
  std::vector<DfaNode> dfa_nodes;

  size_t label_idx = 0;
  states.push_back(ex_re->firstpos);
  auto get_state_idx = [&states](std::unordered_set<int>& s) -> int {
    for (size_t i = 0; i < states.size(); ++i) {
      if (states[i] == s) return i;
    }
    states.push_back(std::move(s));
    return int(states.size() - 1);
  };
  while (label_idx < states.size()) {
    const auto& s = states[label_idx];
    std::optional<u32> is_terminal;
    if (s.find(TERMINATION_INDEX) != s.end()) {
      is_terminal = id;
    }
    DfaNode tmp_node;
    std::get<0>(tmp_node) = is_terminal;
    dfa_nodes.push_back(std::move(tmp_node));
    auto& [_, cur] = dfa_nodes[label_idx];
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
  for (auto& [_, node] : dfa_nodes) {
    node.erase(TERMINATION);
  }

  Dfa dfa(std::move(dfa_nodes));
  dfa.minimize();
  return dfa;
}

Dfa Dfa::from_sv(std::string_view sv, u32 id) {
  auto re = re::Re::parse(sv);
  return from_re(std::move(re));
}

}  // namespace parsergen::dfa
