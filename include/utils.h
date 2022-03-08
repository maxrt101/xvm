#ifndef _XVM_UTILS_H_
#define _XVM_UTILS_H_ 1

#include <string>

namespace xvm {

bool isFileExists(const std::string& name);
bool isNumber(const std::string& str);

} /* namespace xvm */

#endif