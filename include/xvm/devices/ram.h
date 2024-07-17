#ifndef _XVM_DEVICES_RAM_H_
#define _XVM_DEVICES_RAM_H_ 1

#include <xvm/bus.h>

#define XVM_BUS_DEV_RAM_NAME "ram"

namespace xvm {
namespace bus {
namespace device {

class RAM : public Device {
 private:
  uint8_t* m_buffer = nullptr;
  size_t m_size;
  size_t m_baseAddr;

 public:
  RAM(size_t size, size_t base);
  ~RAM();

  size_t getSize();
  uint8_t* getBuffer();
  uint8_t read(size_t address) override;
  void write(size_t address, uint8_t value) override;
};

} /* namespace device */
} /* namespace bus */
} /* namespace xvm */

#endif