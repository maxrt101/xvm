#ifndef _XVM_SYSCALLS_H_
#define _XVM_SYSCALLS_H_ 1

#include "vm.h"

#define VMX_SYSCALL_PUTC            20    // [char] -> []
#define VMX_SYSCALL_READC           21    // [] -> [char]
#define VMX_SYSCALL_READL           22    // [str, len] -> []
#define VMX_SYSCALL_OPEN            30    // [filename, mode] -> [handle]
#define VMX_SYSCALL_CLOSE           31    // [handle] -> []
#define VMX_SYSCALL_READ            32    // [handle, buf, len] -> []
#define VMX_SYSCALL_WRITE           33    // [handle, buf, len] -> []
#define VMX_SYSCALL_FSCTL           60    // [..., cmd] -> [...]
#define VMX_SYSCALL_VMCTL           70    // [..., cmd] -> [...]
#define VMX_SYSCALL_SYSCTL          80    // [..., cmd] -> [...]
#define VMX_SYSCALL_BREAKPOINT      90    // [] -> []

#define VMX_SYSCALL_FS_CLOSE_ALL    0
#define VMX_SYSCALL_FS_CREATE_FILE  1
#define VMX_SYSCALL_FS_CREATE_DIR   2
#define VMX_SYSCALL_FS_COPY         3
#define VMX_SYSCALL_FS_MOVE         4
#define VMX_SYSCALL_FS_REMOVE       5
#define VMX_SYSCALL_FS_LIST         6
#define VMX_SYSCALL_FS_CHDIR        7
#define VMX_SYSCALL_FS_PWD          8

#define VMX_SYSCALL_VM_GET_RAM      1
#define VMX_SYSCALL_VM_EXPAND_RAM   2

#define VMX_SYSCALL_FILE_MODE_RDONLY  1
#define VMX_SYSCALL_FILE_MODE_WRONLY  2
#define VMX_SYSCALL_FILE_MODE_RDWR    4
#define VMX_SYSCALL_FILE_MODE_CREATE  8
#define VMX_SYSCALL_FILE_MODE_APPEND  16

namespace xvm {

void registerSyscalls(VM* vm);

} /* namespace xvm */

#endif