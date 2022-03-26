#include "core/dfa.h"

namespace parsergen::dfa {

void dfs_impl(u32 state_idx, Dfa& dfa, std::vector<bool>& visit,
              std::function<void(u32, DfaNode&)> fn) {
  auto& next = std::get<1>(dfa.nodes[state_idx]);
  for (auto [c, next_idx] : next) {
    if (!visit[next_idx]) {
      visit[next_idx] = true;
      dfs_impl(next_idx, dfa, visit, fn);
    }
  }

  fn(state_idx, dfa.nodes[state_idx]);
}

void dfs(Dfa& dfa, std::function<void(u32, DfaNode&)> fn) {
  std::vector<bool> visit(dfa.nodes.size(), false);
  visit[0] = true;
  dfs_impl(0, dfa, visit, fn);
}

void bfs(Dfa& dfa, std::function<void(u32, DfaNode&)> fn) {
  std::vector<bool> visit(dfa.nodes.size(), false);
  std::vector<u32> stack;
  stack.reserve(dfa.nodes.size());

  stack.push_back(0);
  visit[0] = true;
  while (!stack.empty()) {
    auto cur = stack.back();
    stack.pop_back();
    fn(cur, dfa.nodes[cur]);

    const auto& [_, next] = dfa.nodes[cur];
    for (const auto& [__, next_idx] : next) {
      if (!visit[next_idx]) {
        visit[next_idx] = true;
        stack.push_back(next_idx);
      }
    }
  }
}

// "Compilers: Principles, Techniques and Tools" Algorithm 3.39
template <int DFA_STATE_NUM>
static void minimize_impl(Dfa& dfa) {
  // the extra is the dead state
  using bitset = std::bitset<DFA_STATE_NUM + 1>;
  std::unordered_set<bitset> partition;

  // init partition
  constexpr u32 DEAD_STATE_IDX = DFA_STATE_NUM;
  std::unordered_map<u32, bitset> terminal_states;
  bitset nonterm_states;
  bitset dead_states;
  dead_states.set(DEAD_STATE_IDX);
  for (u32 i = 0; i < (u32)dfa.nodes.size(); ++i) {
    auto& [terminal, next] = dfa.nodes[i];
    if (terminal) {
      terminal_states[terminal.value()].set(i);
    } else {
      nonterm_states.set(i);
    }
  }
  // add into partition
  for (auto& [_, bs] : terminal_states) {
    partition.insert(bs);
  }
  if (nonterm_states.count() > 0) partition.insert(nonterm_states);
  partition.insert(dead_states);

  // start minimize
  std::unordered_set<bitset> new_partition = partition;
  while (true) {
    bool changed = false;
    for (auto& bs : partition) {
      if (bs.count() <= 1) {
        continue;
      }

      // try partition
      for (int a = 0; a < 256; ++a) {
        std::unordered_map<u32, std::unordered_set<u32>> map;
        std::unordered_set<u32> dst_set;
        bitset dst_bs;
        for (u32 state_idx = 0; state_idx < DFA_STATE_NUM; ++state_idx) {
          if (bs[state_idx]) {
            u32 dst_idx;
            if (auto it = std::get<1>(dfa.nodes[state_idx]).find(a);
                it != std::get<1>(dfa.nodes[state_idx]).end()) {
              dst_idx = it->second;
            } else {
              dst_idx = DEAD_STATE_IDX;
            }
            map[dst_idx].insert(state_idx);
            dst_bs.set(dst_idx);
            dst_set.insert(dst_idx);
          }
        }

        std::unordered_map<bitset, bitset> new_group_map;
        for (auto& group : partition) {
          for (auto dst_idx : dst_set) {
            if (group[dst_idx]) {
              for (auto state_idx : map[dst_idx]) {
                new_group_map[group].set(state_idx);
              }
            }
          }
        }

        if (new_group_map.size() > 1) {
          changed = true;
          new_partition.erase(bs);
          for (auto& [_, new_group] : new_group_map) {
            new_partition.insert(new_group);
          }

          break;
        }
      }
    }
    if (!changed) break;

    assert(partition != new_partition);
    assert(partition.size() < new_partition.size());
    partition = new_partition;
  }

  // build new dfa
  std::vector<bitset> final_partition;
  final_partition.reserve(partition.size());
  for (auto it = partition.begin(); it != partition.end();) {
    final_partition.push_back(std::move(partition.extract(it++).value()));
  }
  std::unordered_map<u32, u32> state_to_group_idx;
  for (u32 group_idx = 0; group_idx < (u32)final_partition.size();
       ++group_idx) {
    for (u32 state_idx = 0; state_idx < DFA_STATE_NUM + 1; ++state_idx) {
      if (final_partition[group_idx][state_idx]) {
        state_to_group_idx[state_idx] = group_idx;
      }
    }
  }
  std::vector<std::unordered_map<u8, u32>> next_vec(final_partition.size());
  std::vector<std::optional<u32>> terminal_vec(final_partition.size());
  // start node is always 0
  constexpr u32 START_NODE_IDX = 0;
  bool start_has_been_set = false;

  u32 cur_start_group_idx;
  for (u32 group_idx = 0; group_idx < (u32)final_partition.size();
       ++group_idx) {
    auto& group = final_partition[group_idx];
    if (group[START_NODE_IDX]) {
      assert(!start_has_been_set);
      cur_start_group_idx = group_idx;
      start_has_been_set = true;
    }
    for (u32 state_idx = 0; state_idx < DFA_STATE_NUM + 1; ++state_idx) {
      if (group[state_idx]) {
        if (state_idx != DEAD_STATE_IDX) {
          auto terminal = std::get<0>(dfa.nodes[state_idx]);
          if (terminal) {
            if (terminal_vec[group_idx]) {
              assert(terminal_vec[group_idx] == terminal);
            }
            terminal_vec[group_idx] = terminal;
          } else {
            assert(!terminal_vec[group_idx]);
          }
        }
        for (int a = 0; a < 256; ++a) {
          u32 dst_idx;
          if (state_idx == DEAD_STATE_IDX)
            dst_idx = DEAD_STATE_IDX;
          else {
            if (auto it = std::get<1>(dfa.nodes[state_idx]).find(a);
                it != std::get<1>(dfa.nodes[state_idx]).end()) {
              dst_idx = it->second;
            } else {
              dst_idx = DEAD_STATE_IDX;
            }
          }

          auto dst_group_idx = state_to_group_idx[dst_idx];
          // group_idx ----- a ----> dst_group_idx
          if (next_vec[group_idx].find(a) != next_vec[group_idx].end()) {
            assert(next_vec[group_idx][a] == dst_group_idx);
          }
          next_vec[group_idx][a] = dst_group_idx;
        }

        // only do it once
        // break;
      }
    }
  }

  std::vector<DfaNode> nodes;
  if (cur_start_group_idx == START_NODE_IDX) {
    // just move
    for (size_t i = 0; i < final_partition.size(); ++i) {
      nodes.emplace_back(std::move(terminal_vec[i]), std::move(next_vec[i]));
    }
  } else {
    // swap 0 with cur_start_group_idx
    // 0 -> cur_start_group_idx
    // cur_start_group_idx -> 0
    for (size_t i = START_NODE_IDX; i < final_partition.size(); ++i) {
      auto& next = next_vec[i];
      for (auto& [k, v] : next) {
        if (v == START_NODE_IDX)
          v = cur_start_group_idx;
        else if (v == cur_start_group_idx)
          v = START_NODE_IDX;
      }
    }

    for (size_t i = 0; i < final_partition.size(); ++i) {
      if (i == START_NODE_IDX) {
        nodes.emplace_back(std::move(terminal_vec[cur_start_group_idx]),
                           std::move(next_vec[cur_start_group_idx]));
      } else if (i == cur_start_group_idx) {
        nodes.emplace_back(std::move(terminal_vec[START_NODE_IDX]),
                           std::move(next_vec[START_NODE_IDX]));
      } else {
        nodes.emplace_back(std::move(terminal_vec[i]), std::move(next_vec[i]));
      }
    }
  }

  dfa.nodes = std::move(nodes);
}

void Dfa::minimize() {
  // to make sure no dead state
  this->remove_dead_state();
  // TODO: more efficiency
#define CHECK_SIZE_BEFORE_WORK(N) \
  if (this->nodes.size() <= N) {  \
    minimize_impl<N>(*this);      \
    this->remove_dead_state();    \
    return;                       \
  }

  CHECK_SIZE_BEFORE_WORK(15);
  CHECK_SIZE_BEFORE_WORK(31);
  CHECK_SIZE_BEFORE_WORK(63);
  CHECK_SIZE_BEFORE_WORK(127);
  CHECK_SIZE_BEFORE_WORK(255);
  CHECK_SIZE_BEFORE_WORK(511);
  CHECK_SIZE_BEFORE_WORK(1023);

  ERR_EXIT(this->nodes.size(), "minimize needs dfa.nodes.size() <= 1024");

#undef CHECK_SIZE_BEFORE_WORK
}

void Dfa::remove_dead_state() {
  // remove dead state
  // TODO: more efficient
  std::unordered_map<u32, u32> reindex;

  std::vector<bool> terminable(this->nodes.size(), false);
  std::unordered_set<u32> alive_idx;
  dfs(*this, [&](u32 state_idx, DfaNode& node) {
    auto& [terminal, next] = node;
    if (terminal)
      terminable[state_idx] = true;
    else {
      for (auto [c, next_idx] : next) {
        terminable[state_idx] = terminable[state_idx] | terminable[next_idx];
      }
    }
    if (terminable[state_idx]) {
      alive_idx.insert(state_idx);
    }
  });
  int alive_num = 0;
  bfs(*this, [&](u32 state_idx, DfaNode& node) {
    if (terminable[state_idx]) reindex[state_idx] = alive_num++;
  });
  assert(alive_num == (int)alive_idx.size());

  std::vector<DfaNode> new_nodes(alive_num);
  for (auto idx : alive_idx) {
    new_nodes[reindex[idx]] = std::move(nodes[idx]);
    auto& next = std::get<1>(new_nodes[reindex[idx]]);
    for (auto it = next.begin(); it != next.end();) {
      if (alive_idx.find(it->second) == alive_idx.end()) {
        assert(reindex.find(it->second) == reindex.end());
        it = next.erase(it);
      } else {
        it->second = reindex[it->second];
        ++it;
      }
    }
  }

  nodes = std::move(new_nodes);
}

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
  return from_re(std::move(re), id);
}

// "Compilers: Principles, Techniques and Tools" Algorithm 3.20
// subset construction
template <int NFA_STATE_NUM>
static Dfa from_nfa_impl(nfa::Nfa&& nfa) {
  using bitset = std::bitset<NFA_STATE_NUM>;
  auto e_closure = [&nfa](const bitset& T) {
    bitset bs = T;
    std::vector<u32> stack;
    for (int idx = 0; idx < NFA_STATE_NUM; ++idx) {
      if (T[idx]) stack.push_back(idx);
    }

    while (!stack.empty()) {
      u32 t = stack.back();
      stack.pop_back();
      for (auto u : nfa.nodes[t].eps_edges) {
        if (!bs[u]) {
          bs.set(u);
          stack.push_back(u);
        }
      }
    }
    return bs;
  };

  auto move = [&nfa](const bitset& T, char c) {
    bitset bs;
    for (int idx = 0; idx < NFA_STATE_NUM; ++idx) {
      if (T[idx]) {
        if (auto it = nfa.nodes[idx].edges.find(c);
            it != nfa.nodes[idx].edges.end()) {
          for (auto next_idx : it->second) {
            bs.set(next_idx);
          }
        }
      }
    }
    return bs;
  };

  auto is_terminal = [&nfa](const bitset& T) {
    std::optional<u32> terminal;
    for (int idx = 0; idx < NFA_STATE_NUM; ++idx) {
      if (T[idx]) {
        auto terminal_id = nfa.nodes[idx].terminal_id;
        if (terminal) {
          if (terminal_id) terminal = std::min(terminal, terminal_id);
        } else {
          terminal = terminal_id;
        }
      }
    }
    return terminal;
  };

  std::vector<std::unordered_map<u8, u32>> trans;
  std::vector<std::optional<u32>> terminals;
  std::vector<bitset> unmarked_dfa_state;
  std::unordered_map<bitset, u32> id_link;

  // start_node
  bitset init_node;
  init_node.set(0);
  bitset start_node = e_closure(init_node);
  unmarked_dfa_state.push_back(start_node);
  u32 cur_id = 0;
  id_link[start_node] = cur_id++;
  trans.push_back(std::unordered_map<u8, u32>());
  terminals.push_back(std::move(is_terminal(start_node)));
  assert(trans.size() == cur_id);
  while (!unmarked_dfa_state.empty()) {
    bitset T = unmarked_dfa_state.back();
    unmarked_dfa_state.pop_back();

    for (int a = 0; a < 256; ++a) {
      auto T_a_move = move(T, a);
      auto U = e_closure(T_a_move);
      if (auto it = id_link.find(U); it == id_link.end()) {
        // U not in dfa states
        id_link[U] = cur_id++;
        trans.push_back(std::unordered_map<u8, u32>());
        terminals.push_back(std::move(is_terminal(U)));
        assert(trans.size() == cur_id);
        unmarked_dfa_state.push_back(U);
      }
      trans[id_link[T]][a] = id_link[U];
    }
  }

  assert(trans.size() == cur_id);
  assert(terminals.size() == cur_id);
  std::vector<DfaNode> nodes;
  for (u32 idx = 0; idx < cur_id; ++idx) {
    nodes.emplace_back(std::move(terminals[idx]), std::move(trans[idx]));
  }

  Dfa dfa(std::move(nodes));
  dfa.minimize();
  return dfa;
}

Dfa Dfa::from_nfa(nfa::Nfa&& nfa) {
#define CHECK_SIZE_BEFORE_WORK(N) \
  if (nfa.nodes.size() <= N) return from_nfa_impl<N>(std::move(nfa))

  CHECK_SIZE_BEFORE_WORK(16);
  CHECK_SIZE_BEFORE_WORK(32);
  CHECK_SIZE_BEFORE_WORK(64);
  CHECK_SIZE_BEFORE_WORK(128);
  CHECK_SIZE_BEFORE_WORK(256);
  CHECK_SIZE_BEFORE_WORK(512);
  CHECK_SIZE_BEFORE_WORK(1024);

  ERR_EXIT(nfa.nodes.size(), "from_nfa needs nfa.nodes.size() <= 1024");

#undef CHECK_SIZE_BEFORE_WORK
}
}  // namespace parsergen::dfa
