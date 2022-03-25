#include <fstream>
#include <iostream>
#include <set>
#include <string>

#include "argparse.hpp"
#include "core/dfa.h"
#include "core/re.h"

using namespace parsergen;

/*
 * generate dot file for Regex/Dfa/Nfa
 *
 */

std::string dot_from_re(std::unique_ptr<re::Re> regex, bool verbose) {
  std::ostringstream out;
  out << "digraph g{\n";

  std::unordered_map<int, u8> leafpos_map;
  std::unordered_map<int, std::unordered_set<int>> followpos;

  auto re = regex.get();
  dfa::DfaEngine dfa(std::move(regex));

  std::vector<re::Re*> stack;
  std::function<std::string(const std::unordered_set<int>&)> set_to_string =
      [](const std::unordered_set<int>& s) {
        std::string o = "{";
        std::set<int> new_s(s.begin(), s.end());
        for (auto i : new_s) o += std::to_string(i) + ",";
        if (!new_s.empty()) o.pop_back();
        o += "}";
        return o;
      };

  std::function<std::string(re::Re*, const std::unordered_set<int>&)>
      to_string = [&](re::Re* r, const std::unordered_set<int>& fp) {
        std::string label;
        label +=
            std::string("nullable: ") + (r->nullable ? "true" : "false") + "\n";
        label += std::string("firstpos: ") + set_to_string(r->firstpos) + "\n";
        label += std::string("lastpos: ") + set_to_string(r->lastpos) + "\n";
        if (auto c = dyn_cast<re::Char>(r)) {
          label += std::string("pos: ") + std::to_string(c->leaf_idx) + "\n";
          label += std::string("followpos: ") + set_to_string(fp) + "\n";
        }
        return label;
      };

  stack.push_back(re);
  std::unordered_map<re::Re*, int> idx_map;
  int idx = 0;
  idx_map[re] = idx++;
  while (!stack.empty()) {
    auto top = stack.back();
    auto top_idx = idx_map.at(top);
    stack.pop_back();
    if (auto c = dyn_cast<re::Eps>(top)) {
      std::string label = "Eps";
      if (verbose) {
        label += "\n";
        label += to_string(c, {});
      }
      out << top_idx << " [ shape = circle, label = \"" << label << "\" ]\n";
    } else if (auto c = dyn_cast<re::Char>(top)) {
      std::string label = std::string("Char: ") + c->c;
      if (verbose) {
        label += "\n";
        label += to_string(c, followpos[c->leaf_idx]);
      }
      out << top_idx << " [ shape = circle, label = \"" << label << "\"]\n";
    } else if (auto c = dyn_cast<re::Kleene>(top)) {
      std::string label = "Kleene";
      if (verbose) {
        label += "\n";
        label += to_string(c, {});
      }
      out << top_idx << " [ shape = circle, label = \"" << label << "\" ]\n";
      assert(idx_map.find(c->son.get()) == idx_map.end());
      idx_map[c->son.get()] = idx++;
      stack.push_back(c->son.get());
      out << top_idx << " -> " << idx_map.at(c->son.get()) << "\n";
    } else if (auto c = dyn_cast<re::Concat>(top)) {
      std::string label = "Concat";
      if (verbose) {
        label += "\n";
        label += to_string(c, {});
      }
      out << top_idx << " [ shape = circle, label = \"" << label << "\" ]\n";
      for (auto& son : c->sons) {
        assert(idx_map.find(son.get()) == idx_map.end());
        idx_map[son.get()] = idx++;
        stack.push_back(son.get());
        out << top_idx << " -> " << idx_map.at(son.get()) << "\n";
      }
    } else if (auto c = dyn_cast<re::Disjunction>(top)) {
      std::string label = "Disjunction";
      if (verbose) {
        label += "\n";
        label += to_string(c, {});
      }
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
  delete re;

  return out.str();
}

std::string dot_from_dfa(const dfa::Dfa& dfa) {
  std::ostringstream out;
  out << "digraph g{\n";
  out << "rankdir = \"LR\"\n";

  for (size_t i = 0; i < dfa.nodes.size(); ++i) {
    auto& [node, terminal] = dfa.nodes[i];

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
      out << i << " [ shape = doublecircle, label = \"" << i << "\" ]\n";
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

  parser.add_argument("-v", "--verbose")
      .help("increase output verbosity")
      .default_value(false)
      .implicit_value(true);

  parser.add_argument("-t", "--type")
      .help("generate type [ast, dfa] default: dfa")
      .default_value(std::string{"dfa"})
      .action([](const std::string& value) {
        static const std::vector<std::string> choices = {"ast", "dfa"};
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
  auto verbose = parser["--verbose"] == true;

  std::string result;

  if (type == "dfa") {
    auto [_, dfa] = dfa::DfaEngine::produce(regex);
    result = dot_from_dfa(dfa);
  } else if (type == "ast") {
    auto re = re::ReEngine::produce(regex);
    result = dot_from_re(std::move(re), verbose);
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
