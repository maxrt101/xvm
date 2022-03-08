#include "syscalls.h"
#include "devices/ram.h"
#include "utils.h"
#include "log.h"

#include <iostream>
#include <string>
#include <cstdio>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

/* non blocking read
int flags = fcntl(STDIN_FILENO, F_GETFL);
fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
read(STDERR_FILENO, &c, 1);
fcntl(STDIN_FILENO, F_SETFL, flags | -O_NONBLOCK);
*/

static std::unordered_map<std::string, int> g_fds;


static std::string busReadString(xvm::VM* vm, int32_t ptr) {
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

static int convertFileFlags(int flags) {
  int outflags = 0;
  if (flags & VMX_SYSCALL_FILE_MODE_RDONLY) {
    outflags |= O_RDONLY;
  }
  if (flags & VMX_SYSCALL_FILE_MODE_WRONLY) {
    outflags |= O_WRONLY;
  }
  if (flags & VMX_SYSCALL_FILE_MODE_RDWR) {
    outflags |= O_RDWR;
  }
  if (flags & VMX_SYSCALL_FILE_MODE_CREATE) {
    outflags |= O_CREAT;
  }
  if (flags & VMX_SYSCALL_FILE_MODE_APPEND) {
    outflags |= O_APPEND;
  }
  return outflags;
}


void xvm::registerSyscalls(VM* vm) {
  vm->registerSyscall(VMX_SYSCALL_PUTC, "putc", [](VM* vm) {
    printf("%c", vm->getStack().pop());
  });

  vm->registerSyscall(VMX_SYSCALL_READC, "readc", [](VM* vm) {
    char c = 0;
    termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);

    newt = oldt;
    newt.c_lflag &= ~ICANON;

    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    c = getchar();
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt);

    vm->getStack().push(c);
  });

  vm->registerSyscall(VMX_SYSCALL_READL, "readl", [](VM* vm) {
    int32_t len = vm->getStack().pop();
    int32_t strptr = vm->getStack().pop();

    std::string str;
    std::getline(std::cin, str);

    len = std::max(len, (int32_t)str.size());
    for (int i = 0; i < len; i++) {
      vm->getBus().write(strptr + i, str[i]);
    }
  });

  vm->registerSyscall(VMX_SYSCALL_OPEN, "open", [](VM* vm) {
    int32_t mode = vm->getStack().pop();
    int32_t fileptr = vm->getStack().pop();

    std::string filename = busReadString(vm, fileptr);

    /*if (isFileExists(filename)) {
      vm->getStack().push(-1);
      debug("File '%s' doesn't exist", filename.c_str());
      return;
    }*/

    int fd = open(filename.c_str(), convertFileFlags(mode));
    if (fd == -1) {
      debug("Failed to open file '%s' mode 0x%x:0x%x (errno: %d)", filename.c_str(), mode, convertFileFlags(mode), errno);
    } else {
      g_fds[filename] = fd;
    }
    vm->getStack().push(fd);
  });

  vm->registerSyscall(VMX_SYSCALL_CLOSE, "close", [](VM* vm) {
    int32_t fd = vm->getStack().pop();
    close(fd);
  });

  vm->registerSyscall(VMX_SYSCALL_READ, "read", [](VM* vm) {
    int32_t len = vm->getStack().pop();
    int32_t buffer = vm->getStack().pop();
    int32_t fd = vm->getStack().pop();

    auto ram = (bus::device::RAM*)vm->getBus().getDeviceByAddress(buffer);

    read(fd, ram->getBuffer()+buffer, len);
  });

  vm->registerSyscall(VMX_SYSCALL_WRITE, "write", [](VM* vm) {
    int32_t len = vm->getStack().pop();
    int32_t buffer = vm->getStack().pop();
    int32_t fd = vm->getStack().pop();

    auto ram = (bus::device::RAM*)vm->getBus().getDeviceByAddress(buffer);

    write(fd, ram->getBuffer()+buffer, len);
  });

  vm->registerSyscall(VMX_SYSCALL_FSCTL, "fsctl", [](VM* vm) {
  });

  vm->registerSyscall(VMX_SYSCALL_VMCTL, "vmctl", [](VM* vm) {
  });

  vm->registerSyscall(VMX_SYSCALL_SYSCTL, "sysctl", [](VM* vm) {
  });

  vm->registerSyscall(VMX_SYSCALL_BREAKPOINT, "breakpoint", [](VM* vm) {
    // TODO: repl
  });
}
