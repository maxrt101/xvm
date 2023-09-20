#include <xvm/config.h>
#include <xvm/version.h>

#include <memory>
#include <vector>
#include <map>

static std::map<std::string, std::string> g_config;

void xvm::config::initialize() {
  set("debug", 0);
  set("disasm", 0);
  set("hexdump", 0);
  set("fancy-disasm", 1);
  set("print-symbol-table", 0);
  set("include-symbols", 1);
  set("pic", 1);
  set("ram-size", 2048);
  set("version", XVM_VERSION);
  set("version-major", XVM_VERSION_MAJOR);
  set("version-minor", XVM_VERSION_MINOR);
  set("version-patch", XVM_VERSION_PATCH);
}

bool xvm::config::exists(const std::string& key) {
  return g_config.find(key) != g_config.end();
}

std::string xvm::config::get(const std::string& key) {
  return g_config.at(key);
}

std::string xvm::config::getOr(const std::string& key, const std::string& defaultValue) {
  return exists(key) ? g_config.at(key) : defaultValue;
}

template <>
void xvm::config::set(const std::string& key, const std::string& value) {
  g_config[key] = value;
}

void xvm::config::set(const std::string& key, const char* value) {
  g_config[key] = value;
}

std::map<std::string, std::string>& xvm::config::getAll() {
  return g_config;
}

std::vector<std::string> xvm::config::getKeys() {
  std::vector<std::string> keys;
  for (auto& [k, _] : g_config) {
    keys.push_back(k);
  }
  return keys;
}

std::string xvm::config::format(const std::string& value) {
  std::string result;

  size_t i = 0;
  while (i < value.size()) {
    if (value[i] == '{') {
      std::string key;
      i++;
      while (i < value.size() && value[i] != '}') {
        key += value[i++];
      }
      if (i >= value.size()) {
        fprintf(stderr, "config::format failed: unterminated bracket\n");
        exit(1);
      }
      i++;
      result += get(key);
    } else {
      result += value[i++];
    }
  }

  return result;
}

int xvm::config::asInt(const std::string& key) {
  return std::stoi(g_config[key]);
}

float xvm::config::asFloat(const std::string& key) {
  return std::stof(g_config[key]);
}

bool xvm::config::asBool(const std::string& key) {
  auto val = g_config[key];
  return val == "true" || val == "1" || val == "y" || val == "yes" || val == "on" || val == "enable";
}
