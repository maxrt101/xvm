#ifndef _XVM_BYTECODE_H_
#define _XVM_BYTECODE_H_ 1

#include <cstdint>
#include <string>


namespace xvm {
namespace abi {

enum OpCode : uint8_t {
  NOP,
  HALT,
  RESET,
  PUSH,     // VALUE        IMM
  POP,
  DUP,
  ROL,
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

enum AddressingMode : uint8_t {
  IMM1 = 0b01,
  IMM2 = 0b10,
  IND  = 0b11,
  STK  = 0b00
};

std::string addressingModeToString(AddressingMode mode);
std::string opCodeToString(OpCode opcode);

uint8_t encodeInstruction(const AddressingMode mode, OpCode opcode);
AddressingMode extractMode(uint8_t byte);
OpCode extractOpcode(uint8_t byte);

int disassembleInstruction(const uint8_t* data, int offset = 0);

} /* namespace abi */
} /* namespace xvm */

#endif