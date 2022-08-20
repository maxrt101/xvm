#include <xvm/syscalls.h>
#include <xvm/devices/ram.h>
#include <xvm/utils.h>

std::string xvm::utils::busReadString(xvm::VM* vm, int32_t ptr) {
  std::string str;
  uint8_t c = 0;
  int i = 0;
  
  while (1) {
    c = vm->getBus().read(ptr + i++);
    if (c != 0) {
      str.push_back(c);
    } else {
      break;
    }
  }

  return str;
}

int xvm::utils::getInt(const std::string& str) {
  if (str.size() > 2 && str[1] == 'x') {
    return std::stoi(str, nullptr, 16);
  } else {
    return std::stoi(str);
  }
}

void xvm::registerSyscalls(VM* vm) {
  vm->registerSyscall(VMX_SYSCALL_PUTC,       "putc",       sys_putc);
  vm->registerSyscall(VMX_SYSCALL_READC,      "readc",      sys_readc);
  vm->registerSyscall(VMX_SYSCALL_READL,      "readl",      sys_readl);
  vm->registerSyscall(VMX_SYSCALL_OPEN,       "open",       sys_open);
  vm->registerSyscall(VMX_SYSCALL_CLOSE,      "close",      sys_close);
  vm->registerSyscall(VMX_SYSCALL_READ,       "read",       sys_read);
  vm->registerSyscall(VMX_SYSCALL_WRITE,      "write",      sys_write);
  vm->registerSyscall(VMX_SYSCALL_SLEEP,      "sleep",      sys_sleep);
  vm->registerSyscall(VMX_SYSCALL_FSCTL,      "fsctl",      [](VM* vm) {});
  vm->registerSyscall(VMX_SYSCALL_VMCTL,      "vmctl",      [](VM* vm) {});
  vm->registerSyscall(VMX_SYSCALL_SYSCTL,     "sysctl",     [](VM* vm) {});
  vm->registerSyscall(VMX_SYSCALL_BREAKPOINT, "breakpoint", sys_breakpoint);
  vm->registerSyscall(VMX_SYSCALL_INIT_VIDEO, "init_video", sys_init_video);
}
