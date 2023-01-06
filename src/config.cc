#include <xvm/config.h>
#include <xvm/version.h>

#include <memory>
#include <vector>
#include <map>

struct ValueInfo {
  std::unique_ptr<xvm::config::Value> m_value;
  std::vector<xvm::config::CallbackType> m_callbacks;
};

static std::map<std::string, ValueInfo> g_config;

static void updateConfig(const std::string& name) {
  for (auto& cb : g_config[name].m_callbacks) {
    cb(g_config[name].m_value.get());
  }
}

xvm::config::Value::Value(config::Type type) : m_type(type) {}
xvm::config::Value::~Value() {}
xvm::config::StringValue::StringValue() : Value(config::Type::STRING) {}
xvm::config::FloatValue::FloatValue() : Value(config::Type::FLOAT) {}
xvm::config::IntValue::IntValue() : Value(config::Type::INT) {}
xvm::config::BoolValue::BoolValue() : Value(config::Type::BOOL) {}
xvm::config::VoidPtrValue::VoidPtrValue() : Value(config::Type::VOIDPTR) {}

void xvm::config::initialize() {
  setInt("debug", 0);
  setBool("disasm", false);
  setBool("hexdump", false);
  setBool("include-symbols", true);
  setInt("ram_size", 1024);
  setString("version", XVM_VERSION);
  setInt("version-code", XVM_VERSION_CODE);
  setInt("version-major", XVM_VERSION_MAJOR);
  setInt("version-minor", XVM_VERSION_MINOR);
  setInt("version-patch", XVM_VERSION_PATCH);
}

bool xvm::config::exists(const std::string& name) {
  return g_config.find(name) != g_config.end();
}

xvm::config::Type xvm::config::getType(const std::string& name) {
  if (exists(name)) {
    return g_config[name].m_value->m_type;
  }
  return Type::NONE;
}

void xvm::config::onUpdate(const std::string& name, CallbackType cb) {
  if (!exists(name)) return;
  g_config[name].m_callbacks.push_back(cb);
}

void xvm::config::setFromString(const std::string& name, const std::string& value) {
  if (!exists(name)) return;
  switch (getType(name)) {
    case Type::NONE: {
      break;
    }
    case Type::STRING: {
      setString(name, value);
      break;
    }
    case Type::INT: {
      setInt(name, std::stoi(value));
      break;
    }
    case Type::FLOAT: {
      setFloat(name, std::stof(value));
      break;
    }
    case Type::BOOL: {
      setBool(name, value == "true" || value == "1" || value == "y");
      break;
    }
    case Type::VOIDPTR: {
      /* ? */
      break;
    }
  }
}

std::string xvm::config::getAsString(const std::string& name) {
  if (!exists(name)) return "";
  switch (getType(name)) {
    case Type::NONE:    return "";
    case Type::STRING:  return getString(name);
    case Type::INT:     return std::to_string(getInt(name));
    case Type::FLOAT:   return std::to_string(getInt(name));
    case Type::BOOL:    return getBool(name) ? "true" : "false";
    case Type::VOIDPTR: return std::to_string((unsigned long long)getVoidPtr(name));
  }
  return "";
}

std::string xvm::config::getString(const std::string& name) {
  if (exists(name)) {
    return g_config[name].m_value->as<StringValue>()->m_data;
  }
  return "";
}

int32_t xvm::config::getInt(const std::string& name) {
  if (exists(name)) {
    return g_config[name].m_value->as<IntValue>()->m_data;
  }
  return 0;
}

float xvm::config::getFloat(const std::string& name) {
  if (exists(name)) {
    return g_config[name].m_value->as<FloatValue>()->m_data;
  }
  return 0.0f;
}

bool xvm::config::getBool(const std::string& name) {
  if (exists(name)) {
    return g_config[name].m_value->as<BoolValue>()->m_data;
  }
  return false;
}

void* xvm::config::getVoidPtr(const std::string& name) {
  if (exists(name)) {
    return g_config[name].m_value->as<VoidPtrValue>()->m_data;
  }
  return nullptr;
}

void xvm::config::setString(const std::string& name, const std::string& value) {
  if (!exists(name)) {
    g_config[name].m_value = std::make_unique<StringValue>();
  }
  g_config[name].m_value->as<StringValue>()->m_data = value;
  updateConfig(name);
}

void xvm::config::setInt(const std::string& name, int32_t value) {
  if (!exists(name)) {
    g_config[name].m_value = std::make_unique<IntValue>();
  }
  g_config[name].m_value->as<IntValue>()->m_data = value;
  updateConfig(name);
}

void xvm::config::setFloat(const std::string& name, float value) {
  if (!exists(name)) {
    g_config[name].m_value = std::make_unique<FloatValue>();
  }
  g_config[name].m_value->as<FloatValue>()->m_data = value;
  updateConfig(name);
}

void xvm::config::setBool(const std::string& name, bool value) {
  if (!exists(name)) {
    g_config[name].m_value = std::make_unique<BoolValue>();
  }
  g_config[name].m_value->as<BoolValue>()->m_data = value;
  updateConfig(name);
}

void xvm::config::setVoidPtr(const std::string& name, void* value) {
  if (!exists(name)) {
    g_config[name].m_value = std::make_unique<VoidPtrValue>();
  }
  g_config[name].m_value->as<VoidPtrValue>()->m_data = value;
  updateConfig(name);
}
