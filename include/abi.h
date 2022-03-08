#ifndef _XVM_ABI_H_
#define _XVM_ABI_H_ 1

#include <cstddef>
#include <cstdint>

namespace xvm {
namespace abi {

union N32 {
  uint8_t  u8[4];
  uint16_t u16[2];
  uint32_t u32;
  int8_t   i8[4];
  int16_t  i16[2];
  int32_t  i32;
  float    f32;
};

union N64 {
  uint8_t  u8[8];
  uint16_t u16[4];
  uint32_t u32[2];
  uint64_t u64;
  int8_t   i8[8];
  int16_t  i16[4];
  int32_t  i32[2];
  float    f32[2];
  int64_t  i64;
  double   d;
};

void readInt16(N32& result, const uint8_t* data, size_t offset);
void readInt32(N32& result, const uint8_t* data, size_t offset);

void hexdump(const uint8_t* data, size_t length);

} /* namespace abi */
} /* namespace xvm */

#endif