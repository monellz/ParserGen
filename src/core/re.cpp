#include "core/re.h"

namespace parsergen::re {

Re* Re::clone() const {
  Re* new_re = nullptr;
  switch (kind) {
    default:
      UNREACHABLE();
    case kEps:
      new_re = cast<Eps>(this)->clone_impl();
      break;
    case kChar:
      new_re = cast<Char>(this)->clone_impl();
      break;
    case kKleene:
      new_re = cast<Kleene>(this)->clone_impl();
      break;
    case kConcat:
      new_re = cast<Concat>(this)->clone_impl();
      break;
    case kDisjunction:
      new_re = cast<Disjunction>(this)->clone_impl();
      break;
  }

  return new_re;
}

std::unordered_set<char> ReEngine::_expand_metachar(std::string_view sv) {
  assert(sv[0] == '\\');
  assert(sv.size() == 2);
  std::unordered_set<char> hs;
  switch (sv[1]) {
    case '\\':
    case '(':
    case ')':
    case '[':
    case ']':
    case '.':
    case '|':
    case '*':
    case '+':
    case '?':
    case '{':
    case '}':
    case '^':
    case '$': {
      // escape
      hs.insert(sv[1]);
      break;
    }
    case 'n': {
      hs.insert('\n');
      break;
    }
    case 't': {
      hs.insert('\t');
      break;
    }
    case 's': {
      hs.insert('\n');
      hs.insert('\t');
      hs.insert('\r');
      hs.insert(' ');
      break;
    }
    case 'w': {
      // [A-Za-z0-9_]
      for (char c = 'A'; c <= 'Z'; ++c) hs.insert(c);
      for (char c = 'a'; c <= 'z'; ++c) hs.insert(c);
      for (char c = '0'; c <= '9'; ++c) hs.insert(c);
      hs.insert('_');
      break;
    }
    case 'd': {
      for (char c = '0'; c <= '9'; ++c) hs.insert(c);
      break;
    }
    default: {
      ERR_EXIT(
          sv, "unsupported char for escaping: " + std::string(sv.substr(0, 2)));
    }
  }

  return hs;
}

std::unique_ptr<Re> ReEngine::parse_brackets(std::string_view sv) {
  // []
  std::string_view original_sv = sv;

  std::unordered_set<char> hs;
  std::function<void(char)> update;
  if (sv[0] == '^') {
    for (int i = 0; i < 256; ++i) hs.insert(i);
    sv.remove_prefix(1);
    update = [&](char c) { hs.erase(c); };
  } else {
    update = [&](char c) { hs.insert(c); };
  }

  while (!sv.empty()) {
    if (sv[0] == '\\') {
      if (sv.size() == 1) ERR_EXIT(original_sv, "escaped char is not complete");
      auto chars = _expand_metachar(sv.substr(0, 2));
      for (auto c : chars) update(c);
      sv.remove_prefix(2);
      continue;
    }

    switch (sv[0]) {
      case '(':
      case ')':
      case '[':
      case ']':
      case '.':
      case '|':
      case '*':
      case '+':
      case '?':
      case '{':
      case '}':
      case '^':
      case '$': {
        ERR_EXIT(original_sv, sv[0],
                 "not support unescaped metachar in brackets");
      }
      case '\\': {
        UNREACHABLE();
      }
      case '-': {
        update('-');
        sv.remove_prefix(1);
        break;
      }
      default: {
        if (sv.size() >= 3 && sv[1] == '-' && std::isalpha(sv[2]) &&
            sv[0] <= sv[2]) {
          // look ahead to check '-'
          for (int i = sv[0]; i < sv[2]; ++i) update(i);
          sv.remove_prefix(3);
        } else {
          update(sv[0]);
          sv.remove_prefix(1);
        }
        break;
      }
    }
  }

  auto final_dis = std::make_unique<Disjunction>();
  for (auto c : hs) {
    final_dis->append(new Char(c));
  }

  return final_dis;
}

std::unique_ptr<Re> ReEngine::parse_without_pipe(std::string_view sv) {
  // meta char
  // ()[].|*+\?     use \ to escape metachar
  // we do not support {} ^ $

  std::string_view original_sv = sv;
  auto concat = std::make_unique<Concat>();
  auto& stack = concat->sons;

  auto check_close = [&](char close_char) {
    size_t right_idx = 1;
    while (right_idx < sv.size() && sv[right_idx] != close_char) right_idx++;
    if (right_idx == sv.size()) {
      ERR_EXIT(original_sv, sv,
               "pair not match, need " + std::string(1, close_char));
    }
    return right_idx;
  };

  while (!sv.empty()) {
    if (sv[0] == '\\') {
      if (sv.size() == 1) ERR_EXIT(original_sv, "escaped char is not complete");

      auto chars = _expand_metachar(sv.substr(0, 2));
      auto d = std::make_unique<Disjunction>();
      for (auto c : chars) d->append(new Char(c));
      stack.push_back(std::move(d));
      sv.remove_prefix(2);
      continue;
    }

    switch (sv[0]) {
      case '+': {
        if (stack.empty()) ERR_EXIT(original_sv, "empty plus");
        auto k = std::make_unique<Kleene>(stack.back()->clone());
        stack.push_back(std::move(k));
        sv.remove_prefix(1);
        break;
      }
      case '*': {
        if (stack.empty()) ERR_EXIT(original_sv, "empty kleene");
        Re* bk = stack.back().release();
        stack.pop_back();
        auto k = std::make_unique<Kleene>(bk);
        stack.push_back(std::move(k));
        sv.remove_prefix(1);
        break;
      }
      case '?': {
        if (stack.empty()) ERR_EXIT(original_sv, "empty question mark");
        Re* bk = stack.back().release();
        stack.pop_back();
        auto k = std::make_unique<Disjunction>();
        k->append(bk);
        k->append(new Eps());
        stack.push_back(std::move(k));
        sv.remove_prefix(1);
        break;
      }
      case '.': {
        auto d = std::make_unique<Disjunction>();
        for (int i = 0; i < 256; ++i) {
          d->append(new Char(i));
        }
        sv.remove_prefix(1);
        break;
      }
      case '[': {
        size_t right_idx = check_close(']');
        if (right_idx == 1) {
          // empty
          sv.remove_prefix(2);
          break;
        }

        // [ sv[1]...sv[right_idx - 1] ]
        auto b = parse_brackets(sv.substr(1, right_idx - 1));
        stack.push_back(std::move(b));
        sv.remove_prefix(right_idx + 1);
        break;
      }
      case '(': {
        size_t right_idx = check_close(')');
        if (right_idx == 1) {
          sv.remove_prefix(2);
          break;
        }

        // ( sv[1]...sv[right_idx - 1] )
        auto b = parse_without_pipe(sv.substr(1, right_idx - 1));
        stack.push_back(std::move(b));
        sv.remove_prefix(right_idx + 1);
        break;
      }
      case ']': {
        ERR_EXIT(original_sv, sv, "brackets not match, too many right bracket");
      }
      case ')': {
        ERR_EXIT(original_sv, sv, "brace not match, too many right brace");
      }
      case '\\':
      case '|': {
        UNREACHABLE();
      }
      default: {
        stack.push_back(std::make_unique<Char>(sv[0]));
        sv.remove_prefix(1);
        break;
      }
    }
  }

  return concat;
}

std::unique_ptr<Re> ReEngine::parse(std::string_view sv) {
  std::vector<std::string_view> output = split(sv, "|");
  if (output.size() == 1) return parse_without_pipe(output[0]);

  auto dis = std::make_unique<Disjunction>();
  dis->sons.reserve(output.size());
  for (auto s : output) {
    assert(!s.empty());
    auto one = parse_without_pipe(s);
    dis->sons.push_back(std::move(one));
  }

  return dis;
}

}  // namespace parsergen::re
