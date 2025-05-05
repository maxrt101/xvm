#ifndef _XVM_ABI_H_
#define _XVM_ABI_H_ 1

#include <cstddef>
#include <cstdint>

using u64 = uint64_t;
using i64 = int64_t;
using u32 = uint32_t;
using i32 = int32_t;
using u16 = uint16_t;
using i16 = int16_t;
using u8  = uint8_t;
using i8  = int8_t;
using f64 = double;
using f32 = float;

namespace xvm {
namespace abi {

union N32 {
  u8  _u8[4];
  u16 _u16[2];
  u32 _u32;
  i8  _i8[4];
  i16 _i16[2];
  i32 _i32;
  f32 _f32;
};

union N64 {
  u8  _u8[8];
  u16 _u16[4];
  u32 _u32[2];
  u64 _u64;
  i8  _i8[8];
  i16 _i16[4];
  i32 _i32[2];
  i64 _i64;
  f32 _f32;
  f64 _f64;
};

void readInt16(N32& result, const uint8_t* data, size_t offset);
void readInt32(N32& result, const uint8_t* data, size_t offset);

void hexdump(const uint8_t* data, size_t length);

} /* namespace abi */
} /* namespace xvm */

#endif