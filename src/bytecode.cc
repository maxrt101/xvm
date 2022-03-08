#include "bytecode.h"
#include "abi.h"

std::string xvm::abi::addressingModeToString(AddressingMode mode) {
  switch (mode) {
    case IMM1: return "IMM1";
    case IMM2: return "IMM2";
    case IND:  return "IND";
    case STK:  return "STK";
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

uint8_t xvm::abi::encodeInstruction(const AddressingMode mode, OpCode opcode) {
  return ((uint8_t)mode << 6) | (uint8_t)opcode;
}

xvm::abi::AddressingMode xvm::abi::extractMode(uint8_t byte) {
  return (AddressingMode)((byte & 0b11000000) >> 6);
}

xvm::abi::OpCode xvm::abi::extractOpcode(uint8_t byte) {
  return (OpCode)(byte & 0b00111111);
}

int xvm::abi::disassembleInstruction(const uint8_t* data, int offset) {
  OpCode opcode = extractOpcode(data[offset]);
  AddressingMode mode = extractMode(data[offset]);
  printf("0x%04x | %-8s %-4s ", offset, opCodeToString(opcode).c_str(), addressingModeToString(mode).c_str());
  switch (opcode) {
    case PUSH: {
      N32 result;
      readInt32(result, data, offset+1);
      printf("%d\n", result.i32);
      return offset+5;
    }
    case DEREF8:
    case DEREF16:
    case DEREF32:
    case STORE8:
    case STORE16:
    case STORE32:
    case LOAD8:
    case LOAD16:
    case LOAD32: {
      if (mode == IMM1 || mode == IMM2) {
        N32 result;
        readInt32(result, data, offset+1);
        printf("%d\n", result.i32);
        return offset+5;
      }
      printf("\n");
      return offset+1;
    }
    case ADD:
    case SUB:
    case MUL:
    case DIV:
    case EQU:
    case LT:
    case GT: {
      if (mode == IMM1) {
        N32 result;
        readInt32(result, data, offset+1);
        printf("%d\n", result.i32);
        return offset+5;
      } else if (mode == IMM2) {
        N32 result1, result2;
        readInt32(result1, data, offset+1);
        readInt32(result2, data, offset+5);
        printf("%d %d\n", result1.i32, result2.i32);
        return offset+9;
      }
      printf("\n");
      return offset+1;
    }
    case SHR:
    case SHL: {
      N32 result;
      readInt32(result, data, offset+1);
      printf("%d\n", result.i32);
      return offset+5;
    }
    case AND:
    case OR: {
      if (mode == IMM1) {
        N32 result;
        readInt32(result, data, offset+1);
        printf("%d\n", result.i32);
        return offset+5;
      } else if (mode == IMM2) {
        N32 result1, result2;
        readInt32(result1, data, offset+1);
        readInt32(result2, data, offset+5);
        printf("%d %d\n", result1.i32, result2.i32);
        return offset+9;
      }
      printf("\n");
      return offset+1;
    }
    case JUMP:
    case JUMPT:
    case JUMPF:
    case CALL: {
      if (mode == IMM1 || mode == IMM2) {
        N32 result;
        readInt32(result, data, offset+1);
        // printf("0x%x -> 0x%x", offset, result.i32);
        printf("0x%x\n", result.i32);
        return offset+5;
      }
      printf("\n");
      return offset+1;
    }
    case SYSCALL: { // TODO: ?
      if (mode == IMM1 || mode == IMM2) {
        N32 result;
        readInt32(result, data, offset+1);
        printf("0x%x\n", result.i32); // print syscall name from syscall table
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
