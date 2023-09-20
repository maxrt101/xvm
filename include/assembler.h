#ifndef _XVM_ASSEMBLER_H_
#define _XVM_ASSEMBLER_H_ 1

#include <unordered_map>
#include <string_view>
#include <stdexcept>
#include <charconv>
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

#include <xvm/abi.h>
#include <xvm/bytecode.h>
#include <xvm/executable.h>

namespace xvm {

enum class TokenType : uint8_t {
  IDENTIFIER,
  NUMBER,
  STRING,

  PERCENT,
  COLON,
  DOT,
  AT,
  DOLLAR,
  PLUS,
  MINUS,
  STAR,
};

struct Token {
  TokenType type;
  std::string str;
  int line;

  Token();
  Token(TokenType type, std::string str, int line);
  Token(TokenType type, const char* str, size_t length, int line);

  bool operator==(const Token& t) const;
  bool operator==(const std::string& s) const;

  i32 toNumber();
};

struct LabelMention {
  i32 address = 0;
  u8 argumentNumber;
};

struct Label {
  int32_t address = -1;
  bool isProcedure = false;
  std::vector<LabelMention> mentions;
};

struct Variable {
  enum class Type {
    I8, I16, I32, STR
  };

  struct Mention {
    i32 address;
    u8 argumentNumber;
    bool isDeref = false;
  };

  std::string name;
  i32 address = 0;
  Type type;
  int count = 1;
  std::vector<Mention> mentions;

  int size() const;

  static Type stringToType(const std::string& type);
  static std::string typeToString(Type type);
};

class Assembler {
 private:
  std::string m_filename;
  std::string m_source;
  std::vector<Token> m_tokens;

  std::vector<std::string> m_includeFolders;
  std::vector<std::string> m_included;

  std::vector<u8> m_code;
  std::unordered_map<std::string, Label> m_labels;
  std::unordered_map<std::string, Variable> m_variables;
  std::unordered_map<std::string, int> m_syscalls;
  std::unordered_map<std::string, std::vector<Token>> m_defines;

  std::vector<std::string> m_exported;
  bool m_exportAll = false;

  const char* m_start = nullptr;
  const char* m_current = nullptr;

  bool m_hadError = false;
  int m_line = 1;
  int m_index = 0;

 public:
  Assembler(const std::string& filename, const std::string& source);

  int assemble(Executable& exe, bool includeSymbols = false);

  void setIncludeFolders(std::vector<std::string>& folders);
  void addIncludeFolder(const std::string& folder);

  void includeFile(const std::string& filename);

  void define(const std::string& key, const std::vector<Token>& value);
  void undef(const std::string& key);
  bool isDefined(const std::string& key);
  std::vector<Token>& getDefined(const std::string& key);

 private:
  void asmNote(const char* fmt, ...);
  void asmWarning(const char* fmt, ...);
  void asmError(const char* fmt, ...);
  void asmError(const Token& t, const char* fmt, ...);

  void tokenize();
  void parse();
  void patchLabels();
  void patchVariables();
  void patchAddressingMode(i32 labelAddress, i32 mentionAddress, u8 argumentNumber);

  // int32_t getAddress(u8 arg, i32 flagsOffset); // ???
  int32_t getAddress(u8 argumentNumber = 1);
  Token getNextToken();

  void skipWhitespace();
  bool isAtEnd();
  bool isNextTokenOnSameLine(int index = -1);

  Token identifier();
  Token character();
  Token number();
  Token string();

  void define();
  void undef();

  void pushOpcode(abi::OpCode opcode, abi::AddressingMode mode1, abi::AddressingMode mode2 = abi::_NONE);
  void pushByte(uint8_t value);
  void pushInt16(int16_t value);
  void pushInt32(int32_t value);
};

} /* namespace xvm */

#endif