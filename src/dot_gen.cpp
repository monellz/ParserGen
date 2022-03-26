#include <fstream>
#include <iostream>
#include <set>
#include <string>

#include "argparse.hpp"
#include "core/dfa.h"
#include "core/nfa.h"
#include "core/re.h"

using namespace parsergen;

/*
 * generate dot file for Regex/Dfa/Nfa
 *
 */

std::string dot_from_re(std::unique_ptr<re::Re> regex) {
  std::ostringstream out;
  out << "digraph g{\n";

  std::vector<re::Re*> stack;
  auto re = regex.get();
  stack.push_back(re);
  std::unordered_map<re::Re*, int> idx_map;
  int idx = 0;
  idx_map[re] = idx++;
  while (!stack.empty()) {
    auto top = stack.back();
    auto top_idx = idx_map.at(top);
    stack.pop_back();
    if (dyn_cast<re::Eps>(top)) {
      std::string label = "Eps";
      out << top_idx << " [ shape = circle, label = \"" << label << "\" ]\n";
    } else if (auto c = dyn_cast<re::Char>(top)) {
      std::string label = std::string("Char: ") + c->c;
      out << top_idx << " [ shape = circle, label = \"" << label << "\"]\n";
    } else if (auto c = dyn_cast<re::Kleene>(top)) {
      std::string label = "Kleene";
      out << top_idx << " [ shape = circle, label = \"" << label << "\" ]\n";
      assert(idx_map.find(c->son.get()) == idx_map.end());
      idx_map[c->son.get()] = idx++;
      stack.push_back(c->son.get());
      out << top_idx << " -> " << idx_map.at(c->son.get()) << "\n";
    } else if (auto c = dyn_cast<re::Concat>(top)) {
      std::string label = "Concat";
      out << top_idx << " [ shape = circle, label = \"" << label << "\" ]\n";
      for (auto& son : c->sons) {
        assert(idx_map.find(son.get()) == idx_map.end());
        idx_map[son.get()] = idx++;
        stack.push_back(son.get());
        out << top_idx << " -> " << idx_map.at(son.get()) << "\n";
      }
    } else if (auto c = dyn_cast<re::Disjunction>(top)) {
      std::string label = "Disjunction";
      out << top_idx << " [ shape = circle, label = \"" << label << "\" ]\n";
      for (auto& son : c->sons) {
        assert(idx_map.find(son.get()) == idx_map.end());
        idx_map[son.get()] = idx++;
        stack.push_back(son.get());
        out << top_idx << " -> " << idx_map.at(son.get()) << "\n";
      }
    }
  }

  out << "}\n";
  return out.str();
}

std::string dot_from_dfa(const dfa::Dfa& dfa) {
  std::ostringstream out;
  out << "digraph g{\n";
  out << "rankdir = \"LR\"\n";

  for (size_t i = 0; i < dfa.nodes.size(); ++i) {
    auto& [terminal, node] = dfa.nodes[i];

    std::unordered_map<int, std::set<int>> outmap;

    for (auto it = node.begin(); it != node.end(); ++it) {
      outmap[it->second].insert(it->first);
    }

    for (auto& [out_idx, label_set] : outmap) {
      std::string label_str;
      for (auto label : label_set)
        label_str += std::string(1, char(label)) + ",";
      label_str.pop_back();
      out << i << " -> " << out_idx << " [ label = \"" << label_str
          << "\" ];\n";
    }
    if (terminal) {
      out << i << " [ shape = doublecircle, label = \"" << i
          << " (id: " << terminal.value() << ")"
          << "\" ]\n";
    } else {
      out << i << " [ shape = circle, label = \"" << i << "\" ]\n";
    }
  }
  out << "}\n";
  return out.str();
}

std::string dot_from_nfa(const nfa::Nfa& nfa) {
  std::ostringstream out;
  out << "digraph g{\n";
  out << "rankdir = \"LR\"\n";

  for (size_t i = 0; i < nfa.nodes.size(); ++i) {
    auto& terminal = nfa.nodes[i].terminal_id;
    auto& eps_edges = nfa.nodes[i].eps_edges;
    auto& edges = nfa.nodes[i].edges;

    // -1 means eps
    const int EPS = -1;
    std::unordered_map<int, std::set<int>> outmap;
    for (auto& [k, v] : edges) {
      for (auto out_idx : v) {
        outmap[out_idx].insert(k);
      }
    }
    for (auto out_idx : eps_edges) {
      outmap[out_idx].insert(EPS);
    }

    for (auto& [out_idx, label_set] : outmap) {
      std::string label_str;
      for (auto label : label_set) {
        if (label == EPS) {
          label_str += "eps,";
        } else {
          label_str += std::string(1, char(label)) + ",";
        }
      }
      label_str.pop_back();
      out << i << " -> " << out_idx << " [ label = \"" << label_str
          << "\" ];\n";
    }
    if (terminal) {
      out << i << " [ shape = doublecircle, label = \"" << i
          << " (id: " << terminal.value() << ")"
          << "\" ]\n";
    } else {
      out << i << " [ shape = circle, label = \"" << i << "\" ]\n";
    }
  }
  out << "}\n";
  return out.str();
}

int main(int argc, char* argv[]) {
  argparse::ArgumentParser parser("Dot Generator");
  parser.add_argument("-r", "--regex")
      .required()
      .help("regex with double quotes");

  parser.add_argument("-t", "--type")
      .help("generate type [ast, dfa, nfa] default: dfa")
      .default_value(std::string{"dfa"})
      .action([](const std::string& value) {
        static const std::vector<std::string> choices = {"ast", "dfa", "nfa"};
        if (std::find(choices.begin(), choices.end(), value) != choices.end()) {
          return value;
        }
        return std::string{"ast"};
      });

  parser.add_argument("-o", "--output")
      .help("speficy output file(*.dot) default: stdout");

  try {
    parser.parse_args(argc, argv);
  } catch (const std::runtime_error& err) {
    std::cout << err.what() << std::endl;
    std::cout << parser;
    return 0;
  }

  auto regex = parser.get<std::string>("--regex");
  auto type = parser.get<std::string>("--type");

  std::string result;

  if (type == "nfa") {
    auto re = re::Re::parse(regex);
    auto nfa = nfa::Nfa::from_re(std::move(re));
    result = dot_from_nfa(nfa);
  } else if (type == "dfa") {
    auto re = re::Re::parse(regex);
    auto dfa = dfa::Dfa::from_re(std::move(re));
    result = dot_from_dfa(dfa);
  } else if (type == "ast") {
    auto re = re::Re::parse(regex);
    result = dot_from_re(std::move(re));
  }

  if (auto output_file = parser.present("--output")) {
    std::ofstream out(*output_file, std::ios::out);
    out << result;
    out.close();
  } else {
    std::cout << result;
  }

  return 0;
}
