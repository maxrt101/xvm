#ifndef _XVM_UTILS_H_
#define _XVM_UTILS_H_ 1

#include <vector>
#include <string>

namespace xvm {

bool isFileExists(const std::string& name);
bool isNumber(const std::string& str);
std::vector<std::string> splitString(const std::string& str, char delimiter);

} /* namespace xvm */

#endif