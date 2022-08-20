#include <xvm/syscalls.h>
#include <xvm/log.h>
#include <xvm/devices/video.h>

#include <thread>

void xvm::sys_sleep(VM* vm) {
  int ms = vm->getStack().pop();

  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
