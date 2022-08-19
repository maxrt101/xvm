#include "syscalls.h"
#include "log.h"

#include <unordered_map>
#include <iostream>
#include <string>
#include <cstdio>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

static std::unordered_map<std::string, int> g_fds;

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

void xvm::sys_putc(VM* vm) {
  printf("%c", vm->getStack().pop());
}

void xvm::sys_readc(VM* vm) {
  char c = 0;
  termios oldt, newt;
  tcgetattr(STDIN_FILENO, &oldt);

  newt = oldt;
  newt.c_lflag &= ~ICANON;

  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  c = getchar();
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

  vm->getStack().push(c);
}

void xvm::sys_readl(VM* vm) {
  int32_t len = vm->getStack().pop();
  int32_t strptr = vm->getStack().pop();

  std::string str;
  std::getline(std::cin, str);

  len = std::max(len, (int32_t)str.size());
  for (int i = 0; i < len; i++) {
    vm->getBus().write(strptr + i, str[i]);
  }
}


void xvm::sys_open(VM* vm) {
  int32_t mode = vm->getStack().pop();
  int32_t fileptr = vm->getStack().pop();
  int32_t permissions = 0644;

  if (mode & VMX_SYSCALL_FILE_READ_PERMS) {
    permissions = vm->getStack().pop();
  }

  std::string filename = utils::busReadString(vm, fileptr);

  // std::filesystem::exists
  /*if (isFileExists(filename)) {
    vm->getStack().push(-1);
    debug("File '%s' doesn't exist", filename.c_str());
    return;
  }*/

  int fd = open(filename.c_str(), convertFileFlags(mode), permissions);
  if (fd == -1) {
    debug("Failed to open file '%s' mode 0x%x:0x%x (errno: %d)", filename.c_str(), mode, convertFileFlags(mode), errno);
  } else {
    g_fds[filename] = fd;
  }
  vm->getStack().push(fd);
}

void xvm::sys_close(VM* vm) {
  int32_t fd = vm->getStack().pop();
  close(fd);
}

void xvm::sys_read(VM* vm) {
  int32_t len = vm->getStack().pop();
  int32_t buffer = vm->getStack().pop();
  int32_t fd = vm->getStack().pop();

  auto ram = (bus::device::RAM*)vm->getBus().getDeviceByAddress(buffer);

  read(fd, ram->getBuffer()+buffer, len);
}

void xvm::sys_write(VM* vm) {
  int32_t len = vm->getStack().pop();
  int32_t buffer = vm->getStack().pop();
  int32_t fd = vm->getStack().pop();

  auto ram = (bus::device::RAM*)vm->getBus().getDeviceByAddress(buffer);

  write(fd, ram->getBuffer()+buffer, len);
}
