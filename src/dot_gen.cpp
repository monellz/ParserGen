#include <iostream>

#include "core/re.hpp"

using namespace parsergen;

/*
 * generate dot file for Regex/Dfa/Nfa
 * 
 */ 

std::string dot_gen(std::unique_ptr<re::Re> re) {
  // TODO print dot for re
  return "";
}


int main(int argc, char *argv[]) {
  std::string s = "abcdefg";
  parsergen::re::parse(s);
  return 0;
}