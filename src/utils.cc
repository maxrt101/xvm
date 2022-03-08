#include "utils.h"
#include <sys/stat.h>

bool xvm::isFileExists(const std::string& name) {
  struct stat buffer;
  return stat(name.c_str(), &buffer) == 0; 
}

bool isNumber(const std::string& str) {
  return !str.empty() && std::find_if(str.begin(), str.end(), [](unsigned char c) { return !std::isdigit(c); }) == str.end();
}
