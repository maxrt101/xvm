#ifndef _XVM_UTILS_H_
#define _XVM_UTILS_H_ 1

#include <algorithm>
#include <vector>
#include <string>

namespace xvm {

bool isFileExists(const std::string& name);
bool isNumber(const std::string& str);
std::vector<std::string> splitString(const std::string& str, char delimiter);

void printTableBorder(const std::vector<size_t>& lengths);
void printTableLine(const std::vector<std::string>& line, const std::vector<size_t>& lengths);
void printTable(const std::vector<std::string>& fields, std::vector<std::vector<std::string>>& lines);

template <typename T>
inline void printTable(const std::vector<std::string>& fields, std::vector<T>& values) {
  std::vector<std::vector<std::string>> lines;
  for (auto& value : values) {
    lines.push_back(value.getStringLine());
  }

  printTable(fields, lines);
}

[[noreturn]] void die(int returnCode = 1);

} /* namespace xvm */

#endif