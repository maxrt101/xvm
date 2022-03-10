#include "utils.h"

#include <iostream>
#include <sstream>
#include <sys/stat.h>

bool xvm::isFileExists(const std::string& name) {
  struct stat buffer;
  return stat(name.c_str(), &buffer) == 0; 
}

bool isNumber(const std::string& str) {
  return !str.empty() && std::find_if(str.begin(), str.end(), [](unsigned char c) { return !std::isdigit(c); }) == str.end();
}

std::vector<std::string> xvm::splitString(const std::string& str, char delimiter) {
  std::vector<std::string> result;
  std::istringstream is(str);
  std::string s;
  while (std::getline(is, s, delimiter)) {
    result.push_back(s);
  }
  return result;
}