#include <xvm/bytecode.h>

std::string xvm::abi::addressingModeToString(AddressingMode mode) {
  switch (mode) {
    case _NONE: return "   ";
    case STK:   return "STK";
    case IMM:   return "IMM";
    case ABS:   return "ABS";
    case PRO:   return "PRO";
    case NRO:   return "NRO";
    default:    return "<?>";
  }
}

std::string xvm::abi::opCodeToString(OpCode opcode) {
  switch (opcode) {
    case NOP:     return "nop";
    case HALT:    return "halt";
    case RESET:   return "reset";
    case PUSH:    return "push";
    case POP:     return "pop";
    case DUP:     return "dup";
    case ROL:     return "rol";
    case ROL3:    return "rol3";
    case DEREF8:  return "deref8";
    case DEREF16: return "deref16";
    case DEREF32: return "deref32";
    case STORE8:  return "store8";
    case STORE16: return "store16";
    case STORE32: return "store32";
    case LOAD8:   return "load8";
    case LOAD16:  return "load16";
    case LOAD32:  return "load32";
    case ADD:     return "add";
    case SUB:     return "sub";
    case MUL:     return "mul";
    case DIV:     return "div";
    case EQU:     return "equ";
    case LT:      return "lt";
    case GT:      return "gt";
    case DEC:     return "dec";
    case INC:     return "inc";
    case SHL:     return "shl";
    case SHR:     return "shr";
    case AND:     return "and";
    case OR:      return "or";
    case JUMP:    return "jump";
    case JUMPT:   return "jumpt";
    case JUMPF:   return "jumpf";
    case CALL:    return "call";
    case SYSCALL: return "syscall";
    case RET:     return "ret";
    default:      return "<error>";
  }
}

xvm::abi::Intstruction xvm::abi::encodeInstruction(const AddressingMode mode1, const AddressingMode mode2, const OpCode opcode) {
  return ((u16)((mode1 << 4) | mode2) << 8) | opcode;
}

u8 xvm::abi::encodeFlags(const AddressingMode mode1, const AddressingMode mode2) {
  return (mode1 << 4) | mode2;
}

xvm::abi::AddressingMode xvm::abi::extractModeArg1(u8 flags) {
  return (AddressingMode) ((flags >> 4) & 0xf);
}

xvm::abi::AddressingMode xvm::abi::extractModeArg2(u8 flags) {
  return (AddressingMode) (flags & 0xf);
}

void xvm::abi::decodeIntstruction(const Intstruction instruction, AddressingMode& mode1, AddressingMode& mode2, OpCode& opcode) {
  u8 flags = (instruction >> 8) & 0xff;
  opcode = (OpCode) (instruction & 0xff);
  mode1 = extractModeArg1(flags);
  mode2 = extractModeArg2(flags);
}

int xvm::abi::disassembleInstruction(const uint8_t* data, int offset) {
  AddressingMode mode[] = {
    extractModeArg1(data[offset]),
    extractModeArg2(data[offset])
  };
  OpCode opcode = (OpCode) data[++offset];

  printf("0x%04x | %-8s %-4s %-4s ", offset-1,
    opCodeToString(opcode).c_str(),
    addressingModeToString(mode[0]).c_str(),
    addressingModeToString(mode[1]).c_str());

  switch (opcode) {
    case DEREF8:
    case DEREF16:
    case DEREF32:
    case STORE8:
    case STORE16:
    case STORE32:
    case LOAD8:
    case LOAD16:
    case LOAD32: {
      if (mode[0] == IMM) {
        N32 result;
        readInt32(result, data, offset+1);
        printf("%d\n", result._i32);
        return offset+5;
      }
      printf("\n");
      return offset+1;
    }
    case PUSH:
    case ADD:
    case SUB:
    case MUL:
    case DIV:
    case EQU:
    case LT:
    case GT:
    case AND:
    case OR: {
      for (int i = 0; i < 2; i++) {
        N32 result;
        switch (mode[i]) {
          case IMM:
            readInt32(result, data, offset+1);
            printf("%d", result._i32);
            offset += 5;
            break;
          case ABS:
            readInt32(result, data, offset+1);
            printf("0x%x", result._i32);
            offset += 5;
            break;
          case PRO:
            readInt32(result, data, offset+1);
            printf("0x%x (0x%x)", result._i32, offset + result._i32 + 1);
            offset += 5;
            break;
          case NRO:
            readInt32(result, data, offset+1);
            printf("0x%x (0x%x)", result._i32, offset - result._i32 + 1);
            offset += 5;
            break;
          default:
            break;
        }
      }

      printf("\n");
      return (opcode == PUSH || opcode == EQU) ? offset : offset + 1;
    }
    case SHR:
    case SHL: {
      N32 result;
      readInt32(result, data, offset+1);
      printf("%d\n", result._i32);
      return offset+5;
    }
    case JUMP:
    case JUMPT:
    case JUMPF:
    case CALL: {
      for (int i = 0; i < 2; i++) {
        N32 result;
        switch (mode[i]) {
          case IMM:
            readInt32(result, data, offset+1);
            printf("%d", result._i32);
            offset += 5;
            break;
          case ABS:
            readInt32(result, data, offset+1);
            printf("0x%x", result._i32);
            offset += 5;
            break;
          case PRO:
            readInt32(result, data, offset+1);
            printf("0x%x (0x%x)", result._i32, offset + result._i32 + 1);
            offset += 5;
            break;
          case NRO:
            readInt32(result, data, offset+1);
            printf("0x%x (0x%x)", result._i32, offset - result._i32 + 1);
            offset += 5;
            break;
          default:
            break;
        }
      }
      printf("\n");
      return offset;
    }
    case SYSCALL: {
      if (mode[0] == IMM) {
        N32 result;
        readInt32(result, data, offset+1);
        printf("0x%x\n", result._i32); // TODO: print syscall name from syscall table
        return offset+5;
      }
      printf("\n");
      return offset+1;
    }
    default: {
      printf("\n");
      return offset+1;
    }
  }
  return offset+1;
}
