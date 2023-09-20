#include <xvm/vm.h>
#include <xvm/abi.h>
#include <xvm/log.h>
#include <xvm/config.h>
#include <xvm/syscalls.h>
#include <xvm/devices/ram.h>

xvm::VM::VM(size_t ramSize) : m_ram(ramSize, 0) {
  m_bus.bind(0, ramSize, &m_ram, false);
  registerSyscalls(this);
}

xvm::VM::~VM() {}

void xvm::VM::loadRegion(size_t address, const uint8_t* data, size_t length) {
  for (size_t i = 0; i < length; i++) {
    m_bus.write(address+i, data[i]);
  }
}

void xvm::VM::printRegion(size_t start, size_t length) {
  int printCols = 16;
  bool canPrint = true;
  char buf[16];
  size_t count = start;
  size_t end = start + length;

  while (canPrint) {
    if (count >= end) break;

    memset(&buf, '.', printCols);

    printf("0x%04zx |", count);

    for (int i = 0; i < printCols; i++) {
      printf("%s", (i % (printCols/2) == 0) ? "  " : " ");

      if (count < end) {
        printf("%02x", (unsigned char)m_bus.read(count));
        buf[(count-1) % printCols] = m_bus.read(count++);
      } else {
        printf("  ");
        canPrint = false;
      }
    }

    printf(" | ");

    for (int i = 0; i < printCols; i++) {
      printf("%c", isprint(buf[i]) ? buf[i] : '.');
    }

    printf("\n");
  }
}

void xvm::VM::loadSymbols(const SymbolTable& table) {
  m_symbols = table;
}

xvm::bus::Bus& xvm::VM::getBus() {
  return m_bus;
}

Stack<xvm::VM::StackType>& xvm::VM::getStack() {
  return m_stack;
}

xvm::SymbolTable& xvm::VM::getSymbols() {
  return m_symbols;
}

void xvm::VM::jump(int32_t address) {
  m_ip = address;
}

void xvm::VM::call(int32_t address) {
  pushCall(m_ip);
  jump(address);
}

void xvm::VM::syscall(const std::string& name) {
  for (auto& pair : m_syscalls) {
    if (pair.second.name == name) {
      pair.second.function(this);
      return;
    }
  }

  error("No syscall with name '%s'", name.c_str());
  stop();
}

void xvm::VM::syscall(int32_t number) {
  if (m_syscalls.find(number) == m_syscalls.end()) {
    error("No syscall with number '0x%x'", number);
    stop();
    return;
  }
  m_syscalls[number].function(this);
}

void xvm::VM::registerSyscall(int32_t number, const std::string& name, SyscallType fn) {
  m_syscalls[number] = {name, fn};
}

void xvm::VM::stop() {
  m_running = false;
}

void xvm::VM::reset() {
  m_stack.reset();
  m_callStack.reset();
  m_ip = 0;
}

void xvm::VM::run() {
  using namespace abi;

  m_running = true;

  while (m_running && m_ip < m_bus.max()) {
    uint8_t flags = next();
    uint8_t opcode = next();

    if (config::asInt("debug") > 0) {
      disassembleInstruction(m_ram.getBuffer(), m_ip-2);
    }

    AddressingMode mode[2] {extractModeArg1(flags), extractModeArg2(flags)};
    m_running = executeInstruction(mode, (OpCode) opcode);
  }
}

bool xvm::VM::executeInstruction(const abi::AddressingMode mode[2], const abi::OpCode opcode) {
  using namespace xvm::abi;

  switch (opcode) {
    case HALT: {
      return false;
    }
    case RESET: {
      reset();
      break;
    }
    case NOP: {
      break;
    }
    case PUSH: {
      N32 n;
      readAddr(n, mode[0]);
      m_stack.push(n._i32);
      break;
    }
    case POP: {
      N32 count;
      if (mode[0] == IMM) {
        nextInt32(count);
      } else {
        // TODO: Support STK mode ?
        count._i32 = 1;
      }
      for (int i = 0; i < count._i32; i++) {
        m_stack.pop();
      }
      break;
    }
    case DUP: {
      m_stack.push(m_stack.peek(0));
      break;
    }
    case ROL: {
      StackType val1 = m_stack.pop();
      StackType val2 = m_stack.pop();
      m_stack.push(val1);
      m_stack.push(val2);
      break;
    }
    case ROL3: { // [a, b, c] -> [c, b, a]
      StackType val1 = m_stack.pop();
      StackType val2 = m_stack.pop();
      StackType val3 = m_stack.pop();
      m_stack.push(val1);
      m_stack.push(val2);
      m_stack.push(val3);
      break;
    }
    case DEREF8: {
      N32 addr;
      if (mode[0] == IMM) {
        nextInt32(addr);
      } else if (mode[0] == STK) {
        addr._i32 = m_stack.pop();
      } // TODO: Error Handling
      m_stack.push(m_bus.read(addr._i32));
      break;
    }
    case DEREF16: {
      N32 addr, result;
      if (mode[0] == IMM) {
        nextInt32(addr);
      } else if (mode[0] == STK) {
        addr._i32 = m_stack.pop();
      } // TODO: Error Handling
      readInt16(result, addr._i32);
      m_stack.push(result._i16[0]);
      break;
    }
    case DEREF32: {
      N32 addr, result;
      if (mode[0] == IMM) {
        nextInt32(addr);
      } else if (mode[0] == STK) {
        addr._i32 = m_stack.pop();
      } // TODO: Error Handling
      readInt32(result, addr._i32);
      m_stack.push(result._i32);
      break;
    }
    case STORE8: {
      N32 addr, value;
      value._i32 = m_stack.pop();
      if (mode[1] == IMM) {
        nextInt32(addr);
      } else if (mode[1] == STK) {
        addr._i32 = m_stack.pop();
      } // TODO: Error Handling
      m_bus.write(addr._i32, value._i8[0]);
      break;
    }
    case STORE16: {
      N32 addr, value;
      value._i32 = m_stack.pop();
      if (mode[1] == IMM) {
        nextInt32(addr);
      } else if (mode[1] == STK) {
        addr._i32 = m_stack.pop();
      } // TODO: Error Handling
      writeInt16(value, addr._i32);
      break;
    }
    case STORE32: {
      N32 addr, value;
      value._i32 = m_stack.pop();
      if (mode[1] == IMM) {
        nextInt32(addr);
      } else if (mode[1] == STK) {
        addr._i32 = m_stack.pop();
      } // TODO: Error Handling
      writeInt32(value, addr._i32);
      break;
    }
    case LOAD8: {
      N32 addr;
      if (mode[0] == IMM) {
        nextInt32(addr);
      } else if (mode[0] == STK) {
        addr._i32 = m_stack.pop();
      } // TODO: Error Handling
      m_stack.push(m_bus.read(addr._i32));
      break;
    }
    case LOAD16: {
      N32 addr, value;
      if (mode[0] == IMM) {
        nextInt32(addr);
      } else if (mode[0] == STK) {
        addr._i32 = m_stack.pop();
      } // TODO: Error Handling
      readInt16(value, addr._i32);
      m_stack.push(value._i32);
      break;
    }
    case LOAD32: {
      N32 addr, value;
      if (mode[0] == IMM) {
        nextInt32(addr);
      } else if (mode[0] == STK) {
        addr._i32 = m_stack.pop();
      } // TODO: Error Handling
      readInt32(value, addr._i32);
      m_stack.push(value._i32);
      break;
    }
    case ADD: {
      N32 val[2];
      readArgs(val, mode);
      m_stack.push(val[1]._i32 + val[0]._i32);
      break;
    }
    case SUB: {
      N32 val[2];
      readArgs(val, mode);
      m_stack.push(val[1]._i32 - val[0]._i32);
      break;
    }
    case MUL: {
      N32 val[2];
      readArgs(val, mode);
      m_stack.push(val[1]._i32 * val[0]._i32);
      break;
    }
    case DIV: {
      N32 val[2];
      readArgs(val, mode);
      m_stack.push(val[1]._i32 / val[0]._i32);
      break;
    }
    case EQU: {
      N32 val[2];
      readArgs(val, mode);
      m_stack.push(val[1]._i32 == val[0]._i32);
      break;
    }
    case LT: {
      N32 val[2];
      readArgs(val, mode);
      m_stack.push(val[1]._i32 < val[0]._i32);
      break;
    }
    case GT: {
      N32 val[2];
      readArgs(val, mode);
      m_stack.push(val[1]._i32 > val[0]._i32);
      break;
    }
    case DEC: {
      N32 value;
      readArgNoImm(value, mode[0]);
      m_stack.push(value._i32 - 1);
      break;
    }
    case INC: {
      N32 value;
      readArgNoImm(value, mode[0]);
      m_stack.push(value._i32 + 1);
      break;
    }
    case SHL: {
      N32 value, shift;
      nextInt32(shift);
      readArgNoImm(value, mode[1]); // FIXME: mode[0] or mode[1] ?
      m_stack.push(value._i32 << shift._i32);
      break;
    }
    case SHR: {
      N32 value, shift;
      nextInt32(shift);
      readArgNoImm(value, mode[1]); // FIXME: mode[0] or mode[1] ?
      m_stack.push(value._i32 >> shift._i32);
      break;
    }
    case AND: {
      N32 val[2];
      readArgs(val, mode);
      m_stack.push(val[1]._i32 & val[0]._i32);
      break;
    }
    case OR: {
      N32 val[2];
      readArgs(val, mode);
      m_stack.push(val[1]._i32 | val[0]._i32);
      break;
    }
    case JUMP: {
      N32 addr;
      readAddr(addr, mode[0]);
      jump(addr._i32);
      break;
    }
    case JUMPT: {
      int32_t condition = m_stack.pop();
      N32 addr;
      readAddr(addr, mode[0]);
      if (condition) {
        jump(addr._i32);
      }
      break;
    }
    case JUMPF: {
      int32_t condition = m_stack.pop();
      N32 addr;
      readAddr(addr, mode[0]);
      if (!condition) {
        jump(addr._i32);
      }
      break;
    }
    case CALL: {
      N32 addr;
      readAddr(addr, mode[0]);
      pushCall(m_ip);
      jump(addr._i32);
      break;
    }
    case SYSCALL: {
      N32 number;
      if (mode[0] == IMM) {
        nextInt32(number);
      } else if (mode[0] == STK) {
        number._i32 = m_stack.pop();
      } // TODO: Handle Errors
      if (m_syscalls.find(number._i32) == m_syscalls.end()) {
        error("No syscall with number '0x%x'", number._i32);
        return false;
      }
      m_syscalls[number._i32].function(this);
      break;
    }
    case RET: {
      jump(popCall());
      break;
    }
    default: {
      error("Unknown Instruction '0x%x' (flags: %s %s)", opcode,
        addressingModeToString(mode[0]).c_str(),
        addressingModeToString(mode[1]).c_str());
      return false;
    }
  }

  if (config::asInt("debug") > 0) {
    printf("       [ ");
    for (int i = m_stack.size()-1; i >= 0; i--) {
      printf("%d ", m_stack.peek(i));
    }
    printf("]\n");
  }

  return true;
}

uint8_t xvm::VM::current() const {
  return m_bus.read(m_ip);
}

uint8_t xvm::VM::peekNext() const {
  return m_bus.read(m_ip+1);
}

uint8_t xvm::VM::next() {
  return m_bus.read(m_ip++);
}

void xvm::VM::nextInt16(abi::N32& value) {
  value._u8[0] = next();
  value._u8[1] = next();
}

void xvm::VM::nextInt32(abi::N32& value) {
  value._u8[0] = next();
  value._u8[1] = next();
  value._u8[2] = next();
  value._u8[3] = next();
}

void xvm::VM::readInt16(abi::N32& value, StackType addr) {
  value._u8[0] = m_bus.read(addr);
  value._u8[1] = m_bus.read(addr+1);
}

void xvm::VM::readInt32(abi::N32& value, StackType addr) {
  value._u8[0] = m_bus.read(addr);
  value._u8[1] = m_bus.read(addr+1);
  value._u8[2] = m_bus.read(addr+2);
  value._u8[3] = m_bus.read(addr+3);
}

void xvm::VM::writeInt16(abi::N32& value, StackType addr) {
  m_bus.write(addr, value._u8[0]);
  m_bus.write(addr+1, value._u8[1]);
}

void xvm::VM::writeInt32(abi::N32& value, StackType addr) {
  m_bus.write(addr, value._u8[0]);
  m_bus.write(addr+1, value._u8[1]);
  m_bus.write(addr+2, value._u8[2]);
  m_bus.write(addr+3, value._u8[3]);
}

void xvm::VM::readArg(abi::N32& arg, const abi::AddressingMode mode) {
  using namespace xvm::abi;

  switch (mode) {
    case IMM:
      nextInt32(arg);
      break;
    case STK:
      arg._i32 = m_stack.pop();
      break;
    case ABS:
      nextInt32(arg);
      readInt32(arg, arg._i32);
      break;
    case PRO:
      nextInt32(arg);
      readInt32(arg, m_ip + arg._i32 - 4);
      break;
    case NRO:
      nextInt32(arg);
      readInt32(arg, m_ip - arg._i32 - 4);
      break;
    default:
      arg._i32 = 0;
      break;
  }
}

void xvm::VM::readArgs(abi::N32* args, const abi::AddressingMode mode[2]) {
  using namespace xvm::abi;

  for (int i = 0; i < 2; i++) {
    // readArg(args[i], mode[i]);
    readAddr(args[i], mode[i]);
  }
}

void xvm::VM::readArgNoImm(abi::N32& value, const abi::AddressingMode mode) {
  using namespace xvm::abi;

  if (mode == ABS) {
    nextInt32(value);
    readInt32(value, value._i32);
  } else if (mode == STK) {
    value._i32 = m_stack.pop();
  } /* TODO: Error Handling */
}

void xvm::VM::readAddr(abi::N32& value, const abi::AddressingMode mode) {
  using namespace xvm::abi;

  if (mode == IMM) {
    nextInt32(value);
  } else if (mode == ABS) {
    nextInt32(value);
  } else if (mode == PRO) {
    nextInt32(value);
    value._i32 = m_ip + value._i32 - 4; // -4 - rewind argument length
  } else if (mode == NRO) {
    nextInt32(value);
    value._i32 = m_ip - value._i32 - 4; // 
  } else if (mode == STK) {
    value._i32 = m_stack.pop();
  }
}

void xvm::VM::pushCall(CallStackType value) {
  m_callStack.push(value);
}

xvm::VM::CallStackType xvm::VM::popCall() {
  return m_callStack.pop();
}
