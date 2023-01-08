#include <xvm/bus.h>

xvm::bus::Device::Device(const std::string& name) : m_name(name) {}

xvm::bus::Device::~Device() {}

std::string xvm::bus::Device::getName() const {
  return m_name;
}

xvm::bus::Bus::Dev::~Dev() {
  if (destroy) {
    delete device;
  }
}

bool xvm::bus::Bus::Dev::check(size_t address) const {
  return address >= beginAddr && address <= endAddr;
}

xvm::bus::Bus::Bus() {}

xvm::bus::Bus::~Bus() {}

uint8_t xvm::bus::Bus::read(size_t address) const {
  for (auto& dev : m_devices) {
    if (dev.check(address)) {
      return dev.device->read(address);
    }
  }
  return 0;
}

void xvm::bus::Bus::write(size_t address, uint8_t value) {
  for (auto& dev : m_devices) {
    if (dev.check(address)) {
      dev.device->write(address, value);
      return;
    }
  }
}

void xvm::bus::Bus::bind(size_t address, size_t length, Device* dev, bool destroy) {
  for (auto& dev : m_devices) {
    if (dev.check(address) || dev.check(address+length)) {
      // ERROR //
      return;
    }
  }
  m_min = std::min(m_min, address);
  m_max = std::max(m_max, address+length);
  m_devices.push_back({address, address+length, dev, destroy});
}

xvm::bus::Device* xvm::bus::Bus::getDevice(size_t index) const {
  return m_devices[index].device;
}

xvm::bus::Device* xvm::bus::Bus::getDeviceByAddress(size_t address) const {
  for (auto& dev : m_devices) {
    if (dev.check(address)) {
      return dev.device;
    }
  }
  return nullptr;
}

xvm::bus::Device* xvm::bus::Bus::getDeviceByName(const std::string& name) const {
  for (auto& dev : m_devices) {
    if (dev.device->getName() == name) {
      return dev.device;
    }
  }
  return nullptr;
}

xvm::bus::Bus::Dev xvm::bus::Bus::getDevByName(const std::string& name) {
  for (auto& dev : m_devices) {
    if (dev.device->getName() == name) {
      return dev;
    }
  }
  return {};
}

std::vector<xvm::bus::Bus::Dev> xvm::bus::Bus::getDevs() {
  return m_devices;
}

size_t xvm::bus::Bus::min() const {
  return m_min;
}

size_t xvm::bus::Bus::max() const {
  return m_max;
}
