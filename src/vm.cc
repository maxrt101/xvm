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
  symbols = table;
}

xvm::bus::Bus& xvm::VM::getBus() {
  return m_bus;
}

Stack<xvm::VM::StackType>& xvm::VM::getStack() {
  return m_stack;
}

xvm::SymbolTable& xvm::VM::getSymbols() {
  return symbols;
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
  m_running = true;
  while (m_running && m_ip < m_bus.max()) {
    uint8_t byte = next();
    if (config::getInt("debug") > 0) {
      abi::disassembleInstruction(m_ram.getBuffer(), m_ip-1);
    }
    m_running = executeInstruction(abi::extractMode(byte), abi::extractOpcode(byte));
  }
}

bool xvm::VM::executeInstruction(abi::AddressingMode mode, abi::OpCode opcode) {
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
      nextInt32(n);
      m_stack.push(n.i32);
      break;
    }
    case POP: {
      N32 count;
      if (mode == IMM1 || mode == IMM2) {
        nextInt32(count);
      } else {
        count.i32 = 1;
      }
      for (int i = 0; i < count.i32; i++) {
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
      if (mode == IMM1 || mode == IMM2) {
        nextInt32(addr);
      } else if (mode == STK) {
        addr.i32 = m_stack.pop();
      } // TODO: Error Handling
      m_stack.push(m_bus.read(addr.i32));
      break;
    }
    case DEREF16: {
      N32 addr, result;
      if (mode == IMM1 || mode == IMM2) {
        nextInt32(addr);
      } else if (mode == STK) {
        addr.i32 = m_stack.pop();
      } // TODO: Error Handling
      readInt16(result, addr.i32);
      m_stack.push(result.i16[0]);
      break;
    }
    case DEREF32: {
      N32 addr, result;
      if (mode == IMM1 || mode == IMM2) {
        nextInt32(addr);
      } else if (mode == STK) {
        addr.i32 = m_stack.pop();
      } // TODO: Error Handling
      readInt32(result, addr.i32);
      m_stack.push(result.i32);
      break;
    }
    case STORE8: {
      N32 addr, value;
      value.i32 = m_stack.pop();
      if (mode == IMM1 || mode == IMM2) {
        nextInt32(addr);
      } else if (mode == STK) {
        addr.i32 = m_stack.pop();
      } // TODO: Error Handling
      m_bus.write(addr.i32, value.i8[0]);
      break;
    }
    case STORE16: {
      N32 addr, value;
      value.i32 = m_stack.pop();
      if (mode == IMM1 || mode == IMM2) {
        nextInt32(addr);
      } else if (mode == STK) {
        addr.i32 = m_stack.pop();
      } // TODO: Error Handling
      writeInt16(value, addr.i32);
      break;
    }
    case STORE32: {
      N32 addr, value;
      value.i32 = m_stack.pop();
      if (mode == IMM1 || mode == IMM2) {
        nextInt32(addr);
      } else if (mode == STK) {
        addr.i32 = m_stack.pop();
      } // TODO: Error Handling
      writeInt32(value, addr.i32);
      break;
    }
    case LOAD8: {
      N32 addr;
      if (mode == IMM1 || mode == IMM2) {
        nextInt32(addr);
      } else if (mode == STK) {
        addr.i32 = m_stack.pop();
      } // TODO: Error Handling
      m_stack.push(m_bus.read(addr.i32));
      break;
    }
    case LOAD16: {
      N32 addr, value;
      if (mode == IMM1 || mode == IMM2) {
        nextInt32(addr);
      } else if (mode == STK) {
        addr.i32 = m_stack.pop();
      } // TODO: Error Handling
      readInt16(value, addr.i32);
      m_stack.push(value.i32);
      break;
    }
    case LOAD32: {
      N32 addr, value;
      if (mode == IMM1 || mode == IMM2) {
        nextInt32(addr);
      } else if (mode == STK) {
        addr.i32 = m_stack.pop();
      } // TODO: Error Handling
      readInt32(value, addr.i32);
      m_stack.push(value.i32);
      break;
    }
    case ADD: {
      N32 val1, val2;
      if (mode == IMM1) {
        val2.i32 = m_stack.pop();
        nextInt32(val1);
      } else if (mode == IMM2) {
        nextInt32(val1);
        nextInt32(val2);
      } else if (mode == IND) {
        readInt32(val1, m_stack.pop());
        readInt32(val2, m_stack.pop());
      } else if (mode == STK) {
        val1.i32 = m_stack.pop();
        val2.i32 = m_stack.pop();
      }
      m_stack.push(val2.i32 + val1.i32);
      break;
    }
    case SUB: {
      N32 val1, val2;
      if (mode == IMM1) {
        val2.i32 = m_stack.pop();
        nextInt32(val1);
      } else if (mode == IMM2) {
        nextInt32(val1);
        nextInt32(val2);
      } else if (mode == IND) {
        readInt32(val1, m_stack.pop());
        readInt32(val2, m_stack.pop());
      } else if (mode == STK) {
        val1.i32 = m_stack.pop();
        val2.i32 = m_stack.pop();
      }
      m_stack.push(val2.i32 - val1.i32);
      break;
    }
    case MUL: {
      N32 val1, val2;
      if (mode == IMM1) {
        val2.i32 = m_stack.pop();
        nextInt32(val1);
      } else if (mode == IMM2) {
        nextInt32(val1);
        nextInt32(val2);
      } else if (mode == IND) {
        readInt32(val1, m_stack.pop());
        readInt32(val2, m_stack.pop());
      } else if (mode == STK) {
        val1.i32 = m_stack.pop();
        val2.i32 = m_stack.pop();
      }
      m_stack.push(val2.i32 * val1.i32);
      break;
    }
    case DIV: {
      N32 val1, val2;
      if (mode == IMM1) {
        val2.i32 = m_stack.pop();
        nextInt32(val1);
      } else if (mode == IMM2) {
        nextInt32(val1);
        nextInt32(val2);
      } else if (mode == IND) {
        readInt32(val1, m_stack.pop());
        readInt32(val2, m_stack.pop());
      } else if (mode == STK) {
        val1.i32 = m_stack.pop();
        val2.i32 = m_stack.pop();
      }
      m_stack.push(val2.i32 / val1.i32);
      break;
    }
    case EQU: {
      N32 val1, val2;
      if (mode == IMM1) {
        val2.i32 = m_stack.pop();
        nextInt32(val1);
      } else if (mode == IMM2) {
        nextInt32(val1);
        nextInt32(val2);
      } else if (mode == IND) {
        readInt32(val1, m_stack.pop());
        readInt32(val2, m_stack.pop());
      } else if (mode == STK) {
        val1.i32 = m_stack.pop();
        val2.i32 = m_stack.pop();
      }
      m_stack.push(val2.i32 == val1.i32);
      break;
    }
    case LT: {
      N32 val1, val2;
      if (mode == IMM1) {
        val2.i32 = m_stack.pop();
        nextInt32(val1);
      } else if (mode == IMM2) {
        nextInt32(val1);
        nextInt32(val2);
      } else if (mode == IND) {
        readInt32(val1, m_stack.pop());
        readInt32(val2, m_stack.pop());
      } else if (mode == STK) {
        val1.i32 = m_stack.pop();
        val2.i32 = m_stack.pop();
      }
      m_stack.push(val2.i32 < val1.i32);
      break;
    }
    case GT: {
      N32 val1, val2;
      if (mode == IMM1) {
        val2.i32 = m_stack.pop();
        nextInt32(val1);
      } else if (mode == IMM2) {
        nextInt32(val1);
        nextInt32(val2);
      } else if (mode == IND) {
        readInt32(val1, m_stack.pop());
        readInt32(val2, m_stack.pop());
      } else if (mode == STK) {
        val1.i32 = m_stack.pop();
        val2.i32 = m_stack.pop();
      }
      m_stack.push(val2.i32 > val1.i32);
      break;
    }
    case DEC: {
      N32 value;
      if (mode == IND) {
        readInt32(value, m_stack.pop());
      } else if (mode == STK) {
        value.i32 = m_stack.pop();
      } /* TODO: Error Handling */
      m_stack.push(value.i32 - 1);
      break;
    }
    case INC: {
      N32 value;
      if (mode == IND) {
        readInt32(value, m_stack.pop());
      } else if (mode == STK) {
        value.i32 = m_stack.pop();
      } /* TODO: Error Handling */
      m_stack.push(value.i32 + 1);
      break;
    }
    case SHL: {
      N32 value, shift;
      nextInt32(shift);
      if (mode == IND) {
        readInt32(value, m_stack.pop());
      } else if (mode == STK) {
        value.i32 = m_stack.pop();
      } /* TODO: Error Handling */
      m_stack.push(value.i32 << shift.i32);
      break;
    }
    case SHR: {
      N32 value, shift;
      nextInt32(shift);
      if (mode == IND) {
        readInt32(value, m_stack.pop());
      } else if (mode == STK) {
        value.i32 = m_stack.pop();
      } /* TODO: Error Handling */
      m_stack.push(value.i32 >> shift.i32);
      break;
    }
    case AND: {
      N32 val1, val2;
      if (mode == IMM1) {
        val2.i32 = m_stack.pop();
        nextInt32(val1);
      } else if (mode == IMM2) {
        nextInt32(val1);
        nextInt32(val2);
      } else if (mode == IND) {
        readInt32(val1, m_stack.pop());
        readInt32(val2, m_stack.pop());
      } else if (mode == STK) {
        val1.i32 = m_stack.pop();
        val2.i32 = m_stack.pop();
      }
      m_stack.push(val2.i32 & val1.i32);
      break;
      break;
    }
    case OR: {
      N32 val1, val2;
      if (mode == IMM1) {
        val2.i32 = m_stack.pop();
        nextInt32(val1);
      } else if (mode == IMM2) {
        nextInt32(val1);
        nextInt32(val2);
      } else if (mode == IND) {
        readInt32(val1, m_stack.pop());
        readInt32(val2, m_stack.pop());
      } else if (mode == STK) {
        val1.i32 = m_stack.pop();
        val2.i32 = m_stack.pop();
      }
      m_stack.push(val2.i32 | val1.i32);
      break;
    }
    case JUMP: {
      N32 addr;
      if (mode == IMM1 || mode == IMM2) {
        nextInt32(addr);
      } else if (mode == IND) {
        readInt32(addr, m_stack.pop());
      } else if (mode == STK) {
        addr.i32 = m_stack.pop();
      }
      jump(addr.i32);
      break;
    }
    case JUMPT: {
      int32_t condition = m_stack.pop();
      N32 addr;
      if (mode == IMM1 || mode == IMM2) {
        nextInt32(addr);
      } else if (mode == IND) {
        readInt32(addr, m_stack.pop());
      } else if (mode == STK) {
        addr.i32 = m_stack.pop();
      }
      if (condition) {
        jump(addr.i32);
      }
      break;
    }
    case JUMPF: {
      int32_t condition = m_stack.pop();
      N32 addr;
      if (mode == IMM1 || mode == IMM2) {
        nextInt32(addr);
      } else if (mode == IND) {
        readInt32(addr, m_stack.pop());
      } else if (mode == STK) {
        addr.i32 = m_stack.pop();
      }
      if (!condition) {
        jump(addr.i32);
      }
      break;
    }
    case CALL: {
      N32 addr;
      if (mode == IMM1 || mode == IMM2) {
        nextInt32(addr);
      } else if (mode == IND) {
        readInt32(addr, m_stack.pop());
      } else if (mode == STK) {
        addr.i32 = m_stack.pop();
      }
      pushCall(m_ip);
      jump(addr.i32);
      break;
    }
    case SYSCALL: {
      N32 number;
      if (mode == IMM1 || mode == IMM2) {
        nextInt32(number);
      } else if (mode == STK) {
        number.i32 = m_stack.pop();
      } // TODO: Handle Errors
      if (m_syscalls.find(number.i32) == m_syscalls.end()) {
        error("No syscall with number '0x%x'", number.i32);
        return false;
      }
      m_syscalls[number.i32].function(this);
      break;
    }
    case RET: {
      jump(popCall());
      break;
    }
    default: {
      error("Unknown Instruction '0x%x' (mode: %s)", opcode, addressingModeToString(mode).c_str());
      return false;
    }
  }

  if (config::getInt("debug") > 0) {
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
  value.u8[0] = next();
  value.u8[1] = next();
}

void xvm::VM::nextInt32(abi::N32& value) {
  value.u8[0] = next();
  value.u8[1] = next();
  value.u8[2] = next();
  value.u8[3] = next();
}

void xvm::VM::readInt16(abi::N32& value, StackType addr) {
  value.u8[0] = m_bus.read(addr);
  value.u8[1] = m_bus.read(addr+1);
}

void xvm::VM::readInt32(abi::N32& value, StackType addr) {
  value.u8[0] = m_bus.read(addr);
  value.u8[1] = m_bus.read(addr+1);
  value.u8[2] = m_bus.read(addr+2);
  value.u8[3] = m_bus.read(addr+3);
}

void xvm::VM::writeInt16(abi::N32& value, StackType addr) {
  m_bus.write(addr, value.u8[0]);
  m_bus.write(addr+1, value.u8[1]);
}

void xvm::VM::writeInt32(abi::N32& value, StackType addr) {
  m_bus.write(addr, value.u8[0]);
  m_bus.write(addr+1, value.u8[1]);
  m_bus.write(addr+2, value.u8[2]);
  m_bus.write(addr+3, value.u8[3]);
}

void xvm::VM::pushCall(CallStackType value) {
  m_callStack.push(value);
}

xvm::VM::CallStackType xvm::VM::popCall() {
  return m_callStack.pop();
}
