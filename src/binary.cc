#include <xvm/binary.h>
#include <xvm/config.h>
#include <xvm/log.h>
#include <xvm/abi.h>

#include <cstdio>
#include <errno.h>

void xvm::Executable::toFile(const std::string& filename) {
  FILE* file = fopen(filename.c_str(), "wb");

  if (!file) {
    error("Failed to open/create '%s'", filename.c_str());
    return;
  }

  if (!config::getBool("raw")) {
    abi::N32 n;
    n.u32 = magic;
    fwrite(&n, 1, 4, file);
    n.u32 = version;
    fwrite(&n, 1, 4, file);
  }
  fwrite(data.data(), 1, data.size(), file);

  fclose(file);
}

xvm::Executable xvm::Executable::create(uint32_t magic, uint32_t version, std::vector<uint8_t>& data) {
  Executable exe;
  exe.magic = magic;
  exe.version = version;
  exe.data = data;
  return exe;
}

xvm::Executable xvm::Executable::fromFile(const std::string& filename) {
  Executable exe;
  FILE* file = fopen(filename.c_str(), "rb");
  if (!file) {
    if (errno == 2) {
      error("No such file or directory: '%s'", filename.c_str());
      return exe;
    } else {
      error("Failed to open '%s'", filename.c_str());
      return exe;
    }
  }

  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  rewind(file);

  uint8_t* data = new uint8_t[size];
  if (fread(data, 1, size, file) != size) {
    error("Error: couldn't read file (errno=%d)", errno);
    delete [] data;
    fclose(file);
    return exe;
  }

  if (config::getBool("raw")) {
    exe.data = std::vector(data, data+size);
  } else {
    abi::N32 n;
    n.u8[0] = data[0];
    n.u8[1] = data[1];
    n.u8[2] = data[3];
    n.u8[3] = data[4];
    exe.magic = n.u32;
    n.u8[0] = data[5];
    n.u8[1] = data[6];
    n.u8[2] = data[7];
    n.u8[3] = data[8];
    exe.version = n.u32;

    exe.data = std::vector(data+8, data+size);
  }

  delete [] data;
  fclose(file);
  return exe;
}
