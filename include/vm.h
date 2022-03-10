#ifndef _XVM_VM_H_
#define _XVM_VM_H_ 1

#include <cstdint>

#include "devices/ram.h"
#include "bytecode.h"
#include "stack.h"
#include "bus.h"
#include "abi.h"

#include <unordered_map>
#include <functional>
#include <string>

namespace xvm {

class VM {
 public:
  using StackType = int32_t;
  using CallStackType = uint32_t;
  using SyscallType = std::function<void(VM*)>;

  struct Syscall {
    std::string name;
    SyscallType function;
  };

 private:
  bus::Bus m_bus;
  bus::device::RAM m_ram;
  size_t m_ip = 0;

  Stack<StackType> m_stack;
  Stack<CallStackType> m_callStack;

  std::unordered_map<int32_t, Syscall> m_syscalls;

  bool m_running = false;

 public:
  VM(size_t ramSize = 1024);
  ~VM();

  void loadRegion(size_t address, const uint8_t* data, size_t length);
  void printRegion(size_t start, size_t length);

  void run();
  void stop();
  void reset();

  void jump(int32_t address);
  void call(int32_t address);
  void syscall(const std::string& name);
  void syscall(int32_t number);
  void registerSyscall(int32_t number, const std::string& name, SyscallType fn);

  bus::Bus& getBus();
  Stack<StackType>& getStack();

 private:
  uint8_t current() const;
  uint8_t peekNext() const;
  uint8_t next();

  void nextInt16(abi::N32& value);
  void nextInt32(abi::N32& value);

  void readInt16(abi::N32& value, StackType addr);
  void readInt32(abi::N32& value, StackType addr);
  void writeInt16(abi::N32& value, StackType addr);
  void writeInt32(abi::N32& value, StackType addr);

  void pushCall(CallStackType value);
  CallStackType popCall();

  bool executeInstruction(abi::AddressingMode mode, abi::OpCode opcode);
};

} /* namespace xvm */

#endif