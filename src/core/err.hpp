#pragma once

#include <stdexcept>
#include <string>

namespace parsergen::err {
class BaseErr : public std::exception {
 public:
  std::string msg;
  BaseErr(const std::string& s) : msg(s) {}
  const char* what() const throw() { return msg.c_str(); }
};

class ReErr : public BaseErr {
 public:
  ReErr(std::string_view re, const char* s)
      : BaseErr("(" + std::string(re) + ")" + s) {}
  ReErr(std::string_view re, std::string s)
      : BaseErr("(" + std::string(re) + ")" + s) {}
};

}  // namespace parsergen::err
