#include <xvm/syscalls.h>
#include <xvm/log.h>
#include <xvm/devices/video.h>

#include <cstdlib>

void xvm::sys_init_video(VM* vm) {
  int memStart = vm->getStack().pop();
  int height = vm->getStack().pop();
  int width = vm->getStack().pop();

  if (vm->getBus().getDeviceByName(XVM_BUS_DEV_VIDEO_NAME) == nullptr) {
    vm->getBus().bind(memStart, memStart + 256 + height*width, &vm->m_video, false);
  }

  bus::device::Video* videoDev = static_cast<bus::device::Video*>(vm->getBus().getDeviceByName(XVM_BUS_DEV_VIDEO_NAME));

  if (!videoDev) {
    error("Can't find video device on the bus");
    exit(1);
  }

  videoDev->initialize(width, height, memStart);
}
