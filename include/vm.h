#ifndef _XVM_VM_H_
#define _XVM_VM_H_ 1

#include <xvm/abi.h>
#include <xvm/bus.h>
#include <xvm/stack.h>
#include <xvm/executable.h>
#include <xvm/bytecode.h>
#include <xvm/devices/ram.h>
#include <xvm/devices/video.h>

#include <unordered_map>
#include <functional>
#include <cstdint>
#include <string>

namespace xvm {

class VM {
  friend void sys_init_video(VM*);

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
#ifdef XVM_FEATURE_VIDEO
  bus::device::Video m_video;
#endif /* XVM_FEATURE_VIDEO */

  size_t m_ip = 0;

  Stack<StackType> m_stack;
  Stack<CallStackType> m_callStack;

  std::unordered_map<int32_t, Syscall> m_syscalls;

  SymbolTable m_symbols;

  bool m_running = false;

 public:
  VM(size_t ramSize = 1024);
  ~VM();

  void loadRegion(size_t address, const uint8_t* data, size_t length);
  void printRegion(size_t start, size_t length);
  void loadSymbols(const SymbolTable& table);

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
  SymbolTable& getSymbols();

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

  void readArg(abi::N32& arg, const abi::AddressingMode mode);
  void readArgs(abi::N32* args, const abi::AddressingMode mode[2]);
  void readArgNoImm(abi::N32& value, const abi::AddressingMode mode);
  void readAddr(abi::N32& value, const abi::AddressingMode mode);

  void pushCall(CallStackType value);
  CallStackType popCall();

  bool executeInstruction(const abi::AddressingMode mode[2], const abi::OpCode opcode);
};

} /* namespace xvm */

#endif