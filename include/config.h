#ifndef _XVM_CONFIG_H_
#define _XVM_CONFIG_H_ 1

#include <functional>
#include <vector>
#include <string>
#include <map>
#include <xvm/abi.h>

namespace xvm {
namespace config {

using CallbackType = std::function<void(std::string)>;

void initialize();
bool exists(const std::string& key);
std::string get(const std::string& key);
std::string getOr(const std::string& key, const std::string& defaultValue);
std::map<std::string, std::string>& getAll();
std::vector<std::string> getKeys();
std::string format(const std::string& value);

template <typename T>
void set(const std::string& key, const T& value) {
  set(key, std::to_string(value));
}

template <>
void set(const std::string& key, const std::string& value);

void set(const std::string& key, const char* value);

int asInt(const std::string& key);
float asFloat(const std::string& key);
bool asBool(const std::string& key);

}; /* namespace config */
}; /* namespace xvm */

#endif