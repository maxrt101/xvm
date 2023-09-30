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
  PUSH,     //
  POP,
  DUP,
  ROL,
  ROL3,
  DEREF8,   //
  DEREF16,  //
  DEREF32,  //
  STORE8,   //
  STORE16,  //
  STORE32,  //
  LOAD8,    //
  LOAD16,   //
  LOAD32,   //
  ADD,      //
  SUB,      //
  MUL,      //
  DIV,      //
  EQU,      //
  LT,       //
  GT,       //
  DEC,      //
  INC,      //
  SHL,      //
  SHR,      //
  AND,      //
  OR,       //
  JUMP,     //
  JUMPT,    //
  JUMPF,    //
  CALL,     //
  SYSCALL,  //
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