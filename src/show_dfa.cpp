#include <iostream>

#include "core/re.hpp"


int main(int argc, char *argv[]) {
  std::string s = "abcdefg";
  parsergen::re::parse(s);
  return 0;
}