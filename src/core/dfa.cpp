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

void Dfa::minimize() {
  // now only remove dead state
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
  return from_re(std::move(re));
}

// "Compilers: Principles, Techniques and Tools" Algorithm 3.20
// subset construction
template <int NFA_STATE_NUM>
static Dfa from_nfa_impl(nfa::Nfa&& nfa) {
  using bitset = std::bitset<NFA_STATE_NUM>;
  auto e_closure_s = [&nfa](u32 state_idx) {
    bitset bs;
    bs.set(state_idx);
    for (auto idx : nfa.nodes[state_idx].eps_edges) bs.set(idx);
    return bs;
  };

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
  bitset start_node = e_closure_s(0);
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

#undef CHECK_SIZE
}
}  // namespace parsergen::dfa
