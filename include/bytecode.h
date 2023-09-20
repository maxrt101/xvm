#ifndef _XVM_BYTECODE_H_
#define _XVM_BYTECODE_H_ 1

#include <cstdint>
#include <string>
#include <xvm/abi.h>

namespace xvm {
namespace abi {

using Intstruction = u16;

enum OpCode : u8 {
  NOP,
  HALT,
  RESET,
  PUSH,     // VALUE        IMM
  POP,
  DUP,
  ROL,
  ROL3,
  DEREF8,   // [ADDR]       IMM     STK
  DEREF16,  // [ADDR]       IMM     STK
  DEREF32,  // [ADDR]       IMM     STK
  STORE8,   // [ADDR]       IMM     STK
  STORE16,  // [ADDR]       IMM     STK
  STORE32,  // [ADDR]       IMM     STK
  LOAD8,    // [ADDR]       IMM     STK
  LOAD16,   // [ADDR]       IMM     STK
  LOAD32,   // [ADDR]       IMM     STK
  ADD,      // [OP1 [OP2]]  IMM IND STK
  SUB,      // [OP1 [OP2]]  IMM IND STK
  MUL,      // [OP1 [OP2]]  IMM IND STK
  DIV,      // [OP1 [OP2]]  IMM IND STK
  EQU,      // [OP1 [OP2]]  IMM IND STK
  LT,       // [OP1 [OP2]]  IMM IND STK
  GT,       // [OP1 [OP2]]  IMM IND STK
  DEC,      // [ADDR]           IND STK
  INC,      // [ADDR]           IND STK
  SHL,      // [ADDR]           IND STK
  SHR,      // [ADDR]           IND STK
  AND,      // [OP1 [OP2]]  IMM IND STK
  OR,       // [OP1 [OP2]]  IMM IND STK
  JUMP,     // [ADDR/LABEL] IMM IND STK
  JUMPT,    // [ADDR/LABEL] IMM IND STK
  JUMPF,    // [ADDR/LABEL] IMM IND STK
  CALL,     // [ADDR/LABEL] IMM IND STK
  SYSCALL,  // NUMBER       IMM
  RET
};

enum AddressingMode : u8 {
  _NONE = 0,
  STK = 0b0001, // Stack
  IMM = 0b0010, // Immediate
  ABS = 0b0011, // Absolute (Indirect)
  PRO = 0b0100, // Positive Relative Offset (Indirect)
  NRO = 0b0101, // Negative Relative Offset (Indirect)
};

std::string addressingModeToString(AddressingMode mode);
std::string opCodeToString(OpCode opcode);

Intstruction encodeInstruction(const AddressingMode mode1, const AddressingMode mode2, const OpCode opcode);
u8 encodeFlags(const AddressingMode mode1, const AddressingMode mode2);
AddressingMode extractModeArg1(u8 flags);
AddressingMode extractModeArg2(u8 flags);
void decodeIntstruction(const Intstruction instruction, AddressingMode& mode1, AddressingMode& mode2, OpCode& opcode);

int disassembleInstruction(const uint8_t* data, int offset = 0);

} /* namespace abi */
} /* namespace xvm */

#endif