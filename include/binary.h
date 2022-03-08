#ifndef _XVM_BINARY_H_
#define _XVM_BINARY_H_ 1

#include <cstdint>
#include <string>
#include <vector>

#define XVM_MAGIC 0xdeadbeef
#define XVM_BAD_MAGIC -1U

namespace xvm {

struct Executable {
  uint32_t magic = XVM_BAD_MAGIC;
  uint32_t version = 0;
  std::vector<uint8_t> data;

  void toFile(const std::string& filename);

  static Executable create(uint32_t magic, uint32_t version, std::vector<uint8_t>& data);
  static Executable fromFile(const std::string& filename);
};

} /* namespace xvm */

#endif