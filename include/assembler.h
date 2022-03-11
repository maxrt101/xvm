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

#include "bytecode.h"
#include "binary.h"

namespace xvm {

enum class TokenType : uint8_t {
  IDENTIFIER,
  NUMBER,
  STRING,

  PERCENT,
  COLON,
  DOT,
  AT,
  PLUS,
  MINUS,
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

  int32_t toNumber();

  /*template <typename T>
  inline T toNumber() const {
    int base = 10;
    if (str.size() > 2 && str[2] == 'b') base = 2;
    if (str.size() > 2 && str[2] == 'x') base = 16;
    T value = 0;
    auto result = std::from_chars(str.data()+2, str.data() + str.size(), value, base);
    if (result.ec == std::errc::invalid_argument) throw std::logic_error("Invalid number");
    return value;
  }*/
};

struct LabelMention {
  int32_t address = 0;
};

struct Label {
  int32_t address = -1;
  std::vector<LabelMention> mentions;
};

/* struct Variable {
  enum class Type {
    I8, I16, I32
  };
  
  int32_t address = 0;
  Type type;
  std::vector<> mentions;
}; */

class Assembler {
 private:
  std::string m_filename;
  std::string m_source;
  std::vector<Token> m_tokens;

  std::vector<std::string> m_includeFolders;
  std::vector<std::string> m_included;

  std::vector<uint8_t> m_code;
  std::unordered_map<std::string, Label> m_labels;
  std::unordered_map<std::string, int> m_syscalls;
  std::unordered_map<std::string, std::vector<Token>> m_defines;

  const char* m_start = nullptr;
  const char* m_current = nullptr;

  bool m_hadError = false;
  int m_line = 1;
  int m_index = 0;

 public:
  Assembler(const std::string& filename, const std::string& source);

  int assemble(Executable& exe);

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

  int32_t getAddress();
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

  void pushOpcode(abi::AddressingMode mode, abi::OpCode opcode);
  void pushByte(uint8_t value);
  void pushInt16(int16_t value);
  void pushInt32(int32_t value);
};

} /* namespace xvm */

#endif