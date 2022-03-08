#ifndef _XVM_BUS_H_
#define _XVM_BUS_H_ 1

#include <vector>

namespace xvm {
namespace bus {

class Device {
 public:
  Device();
  virtual ~Device();

  virtual uint8_t read(size_t address) = 0;
  virtual void write(size_t address, uint8_t value) = 0;
};

class Bus {
 public:
  struct Dev {
    size_t beginAddr = 0;
    size_t endAddr = 0;
    Device* device = nullptr;
    bool destroy = false;

    ~Dev();

    bool check(size_t address) const;
  };

 private:
  std::vector<Dev> m_devices;
  size_t m_min = 0, m_max = 0;

 public:
  Bus();
  ~Bus();

  uint8_t read(size_t address) const;
  void write(size_t address, uint8_t value);

  void bind(size_t address, size_t length, bus::Device* dev, bool destroy = false);

  Device* getDevice(size_t index) const;
  Device* getDeviceByAddress(size_t address) const;

  size_t min() const;
  size_t max() const;
};

} /* namespace bus */
} /* namespace xvm */

#endif