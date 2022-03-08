#ifndef _XVM_CONFIG_H_
#define _XVM_CONFIG_H_ 1

#include <functional>
#include <string>
#include "abi.h"


namespace xvm {
namespace config {

enum class Type {
  NONE,       // Doesn't exist
  STRING,
  INT,
  FLOAT,
  BOOL,
  VOIDPTR
};

struct Value {
  config::Type m_type;

  Value(config::Type type);
  virtual ~Value();

  template <typename T>
  inline T* as() {
    return (T*)this;
  }
};

struct StringValue : public Value {
  std::string m_data;

  StringValue();
};

struct FloatValue : public Value {
  float m_data;

  FloatValue();
};

struct IntValue : public Value {
  int m_data;

  IntValue();
};

struct BoolValue : public Value {
  bool m_data;

  BoolValue();
};

struct VoidPtrValue : public Value {
  void* m_data;

  VoidPtrValue();
};

using CallbackType = std::function<void(Value*)>;

void initialize();
bool exists(const std::string& name);
Type getType(const std::string& name);
void onUpdate(const std::string& name, CallbackType cb);

void setFromString(const std::string& name, const std::string& value);

std::string getString(const std::string& name);
int32_t getInt(const std::string& name);
float getFloat(const std::string& name);
bool getBool(const std::string& name);
void* getVoidPtr(const std::string& name);

void setString(const std::string& name, const std::string& value);
void setInt(const std::string& name, int32_t value);
void setFloat(const std::string& name, float value);
void setBool(const std::string& name, bool value);
void setVoidPtr(const std::string& name, void* value);

}; /* namespace config */
}; /* namespace xvm */

#endif