#include "abi.h"

#include <cstring>
#include <cstdio>
#include <cctype>

void xvm::abi::readInt16(N32& result, const uint8_t* data, size_t offset) {
  result.u8[0] = data[offset];
  result.u8[1] = data[offset+1];
}

void xvm::abi::readInt32(N32& result, const uint8_t* data, size_t offset) {
  result.u8[0] = data[offset];
  result.u8[1] = data[offset+1];
  result.u8[2] = data[offset+2];
  result.u8[3] = data[offset+3];
}

void xvm::abi::hexdump(const uint8_t* data, size_t length) {
  int printCols = 16;
  bool canPrint = true;
  char buf[16];
  size_t count = 0;
  size_t end = length;

  while (canPrint) {
    if (count >= end) break;

    memset(&buf, '.', printCols);

    printf("0x%04zx |", count);

    for (int i = 0; i < printCols; i++) {
      printf("%s", (i % (printCols/2) == 0) ? "  " : " ");

      if (count < end) {
        printf("%02x", (unsigned char)data[count]);
        buf[count % printCols] = data[count++];
      } else {
        printf("  ");
        canPrint = false;
      }
    }

    printf(" | ");

    for (int i = 0; i < printCols; i++) {
      printf("%c", isprint(buf[i]) ? buf[i] : '.');
    }

    printf("\n");
  }
}
