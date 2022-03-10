#include "devices/ram.h"

#include <cstdio>

xvm::bus::device::RAM::RAM(size_t size, size_t base) : m_size(size), m_baseAddr(base) {
  m_buffer = new uint8_t[size] {0};
}

xvm::bus::device::RAM::~RAM() {
  delete [] m_buffer;
  m_buffer = nullptr;
}

size_t xvm::bus::device::RAM::getSize() {
  return m_size;
}

uint8_t* xvm::bus::device::RAM::getBuffer() {
  return m_buffer;
}

uint8_t xvm::bus::device::RAM::read(size_t address) {
  return m_buffer[address-m_baseAddr];
}

void xvm::bus::device::RAM::write(size_t address, uint8_t value) {
  m_buffer[address-m_baseAddr] = value;
}
