#include <xvm/utils.h>

#include <iostream>
#include <sstream>
#include <sys/stat.h>

bool xvm::isFileExists(const std::string& name) {
  struct stat buffer;
  return stat(name.c_str(), &buffer) == 0; 
}

bool xvm::isNumber(const std::string& str) {
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

void xvm::printTableBorder(const std::vector<size_t>& lengths) {
  for (int i = 0; i < lengths.size(); i++) {
    printf("+");
    for (int j = 0; j < lengths[i]+2; j++) {
      printf("-");
    }
  }
  printf("+\n");
}

void xvm::printTableLine(const std::vector<std::string>& line, const std::vector<size_t>& lengths) {
  printf("| ");
  for (size_t i = 0; i < line.size(); i++) {
    printf("%s", line[i].c_str());
    size_t rest = lengths[i] - line[i].size();
    for (int i = 0; i < rest; i++) {
      printf(" ");
    }
    printf(" | ");
  }
  printf("\n");
}

void xvm::printTable(const std::vector<std::string>& fields, std::vector<std::vector<std::string>>& lines) {
  std::vector<size_t> lengths(fields.size());

  for (int i = 0; i < fields.size(); i++) {
    std::vector<size_t> columns;
    std::transform(lines.begin(), lines.end(), std::back_inserter(columns),
      [&i](const std::vector<std::string>& line) { return line[i].size(); });
    if (columns.empty()) {
      lengths[i] = fields[i].size();
    } else {
      lengths[i] = std::max(*std::max_element(columns.begin(), columns.end()), fields[i].size());
    }
  }

  printTableBorder(lengths);
  printTableLine(fields, lengths);
  printTableBorder(lengths);
  for (size_t i = 0; i < lines.size(); i++) {
    printTableLine(lines[i], lengths);
  }
  printTableBorder(lengths);
}

[[noreturn]] void xvm::die(int returnCode) {
  exit(returnCode);
}
