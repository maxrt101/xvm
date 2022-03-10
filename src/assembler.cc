#include "assembler.h"
#include "version.h"
#include "utils.h"
#include "log.h"
#include "abi.h"

#include <algorithm>
#include <iostream>
#include <fstream>

#define XVM_IFDEF_TEMPLATE(directive, equ) \
  do {                                                                              \
    if (!isNextTokenOnSameLine()) {                                                 \
      asmError("Expected identifier after " directive);                             \
      return;                                                                       \
    }                                                                               \
    Token name = m_tokens[++m_index]; /* define is not expanded */                  \
    m_index -= 2;                                                                   \
    m_tokens.erase(m_tokens.begin() + m_index, m_tokens.begin() + m_index + 3);     \
    int index = m_index - 1;                                                        \
    bool erase = m_defines.find(name.str) equ m_defines.end(), parsing = true;      \
    while (parsing && m_index < m_tokens.size()) {                                  \
      if (m_tokens[m_index].type == TokenType::PERCENT) {                           \
        if (m_tokens[m_index+1].str == "else") {                                    \
          erase = !erase;                                                           \
        } else if (m_tokens[m_index+1].str == "endif") {                            \
          parsing = false;                                                          \
        } else if (m_tokens[m_index+1].str == "define") {                           \
          define();                                                                 \
        } else if (m_tokens[m_index+1].str == "undef") {                            \
          undef();                                                                  \
        } else {                                                                    \
          asmError("Unexistent/unsupported directive: '%s'",                        \
            m_tokens[m_index+1].str.c_str());                                       \
          return;                                                                   \
        }                                                                           \
        m_tokens.erase(m_tokens.begin() + m_index, m_tokens.begin() + m_index + 2); \
        m_index--;                                                                  \
      } else if (erase) {                                                           \
        m_tokens.erase(m_tokens.begin() + m_index--);                               \
      }                                                                             \
      m_index++;                                                                    \
    }                                                                               \
    m_index = index;                                                                \
  } while (0)


static std::string g_eof = "EOF";


xvm::Token::Token() {}

xvm::Token::Token(TokenType type, std::string str, int line) : type(type), str(str), line(line) {}

xvm::Token::Token(TokenType type, const char* str, size_t length, int line) : type(type), line(line) {
  this->str = std::string(str, length);
}

bool xvm::Token::operator==(const Token& t) const {
  return str == t.str;
}

bool xvm::Token::operator==(const std::string& s) const {
  return str == s;
}

int32_t xvm::Token::toNumber() {
  int base = 10;
  if (str.size() > 2 && str[1] == 'b') base = 2;
  if (str.size() > 2 && str[1] == 'x') base = 16;
  return std::stoi(base != 10 ? str.substr(2) : str, nullptr, base);
}

xvm::Assembler::Assembler(const std::string& filename, const std::string& source) : m_filename(filename), m_source(source) {
  m_start = m_source.c_str();
  m_current = m_start;
}

int xvm::Assembler::assemble(Executable& exe) {
  tokenize();
  if (m_hadError) return -1;

  parse();
  if (m_hadError) return -1;

  patchLabels();

  exe.magic = XVM_MAGIC;
  exe.version = XVM_VERSION_CODE;
  exe.data = m_code;

  return 0;
}

void xvm::Assembler::setIncludeFolders(std::vector<std::string>& folders) {
  m_includeFolders = folders;
  for (std::string& folder : m_includeFolders) {
    if (folder.back() != '/') {
      folder.push_back('/');
    }
  }
}

void xvm::Assembler::addIncludeFolder(const std::string& folder) {
  m_includeFolders.push_back(folder);
  if (m_includeFolders.back().back() != '/') {
    m_includeFolders.back().push_back('/');
  }
}

void xvm::Assembler::includeFile(const std::string& filename) {
  if (std::find(m_included.begin(), m_included.end(), filename) != m_included.end()) {
    return;
  }
  m_included.push_back(filename);
  std::ifstream sourceFile(filename);
  std::string source((std::istreambuf_iterator<char>(sourceFile)), std::istreambuf_iterator<char>());
  Assembler assembler(filename, source);
  assembler.tokenize();
  m_tokens.insert(m_tokens.begin()+m_index+1, assembler.m_tokens.begin(), assembler.m_tokens.end()-1);
}

void xvm::Assembler::define(const std::string& key, const std::vector<Token>& value) {
  m_defines[key] = value;
}

void xvm::Assembler::undef(const std::string& key) {
  auto itr = m_defines.find(key);
  if (itr != m_defines.end()) {
    m_defines.erase(itr);
  }
}

bool xvm::Assembler::isDefined(const std::string& key) {
  return m_defines.find(key) != m_defines.end();
}

std::vector<xvm::Token>& xvm::Assembler::getDefined(const std::string& key) {
  return m_defines[key];
}

void xvm::Assembler::define() {
  if (!isNextTokenOnSameLine()) {
    asmError("Expected identifier after define");
    return;
  }
  Token name = getNextToken();
  std::vector<Token> value;
  while (isNextTokenOnSameLine()) value.push_back(getNextToken());
  define(name.str, value);
}

void xvm::Assembler::undef() {
  if (!isNextTokenOnSameLine()) {
    asmError("Expected identifier after undef");
    return;
  }
  undef(getNextToken().str);
}

void xvm::Assembler::asmNote(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  printf("%s:%d %sNote%s: ", m_filename.c_str(), m_line, colors::BLUE, colors::RESET);
  vprintf(fmt, args);
  printf("\n");
  va_end(args);
}

void xvm::Assembler::asmWarning(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, "%s:%d %sWarning%s: ", m_filename.c_str(), m_line, colors::YELLOW, colors::RESET);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);
}

void xvm::Assembler::asmError(const char* fmt, ...) {
  m_hadError = true;
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, "%s:%d %sError%s: ", m_filename.c_str(), m_line, colors::RED, colors::RESET);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);
}

void xvm::Assembler::asmError(const Token& t, const char* fmt, ...) {
  m_hadError = true;
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, "%s:%d Error near '%.*s': ", m_filename.c_str(), t.line, static_cast<int>(t.str.size()), t.str.data());
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);
}

void xvm::Assembler::skipWhitespace() {
  while (!isAtEnd() && (*m_current == ' ' || *m_current == '\t')) m_current++;
  m_start = m_current;
}

bool xvm::Assembler::isAtEnd() {
  return *m_current == '\0' || *m_current == EOF;
}

bool xvm::Assembler::isNextTokenOnSameLine(int index) {
  if (index == -1) {
    return m_tokens[m_index].line == m_tokens[m_index+1].line;
  } else {
    return m_tokens[index].line == m_tokens[index+1].line;
  }
}

xvm::Token xvm::Assembler::identifier() {
  m_start = m_current;
  skipWhitespace();
  while (isalnum(*m_current) || *m_current == '_') m_current++;
  return Token(TokenType::IDENTIFIER, m_start, m_current - m_start, m_line);
}

xvm::Token xvm::Assembler::character() {
  m_start = m_current;
  char c = '\0';
  if (*m_current == '\'') m_current++;

  if (*m_current == '\\') {
    switch (*(++m_current)) {
      case '\\':
        c = '\\';
        break;
      case 'n':
        c = '\n';
        break;
      case 't':
        c = '\t';
        break;
      case 'r':
        c = '\r';
        break;
      default:
        c = *m_current;
        break;
    }
  } else {
    c = *m_current;
  }

  if (*(++m_current) != '\'') {
    asmError("Expected ' at the end of char");
  }
  return Token(TokenType::NUMBER, std::to_string((int)c), m_line);
}

xvm::Token xvm::Assembler::number() {
  m_start = m_current;
  skipWhitespace();
  while (isdigit(*m_current) || *m_current == 'x' || *m_current == 'b' || (*m_current >= 'a' && *m_current <= 'f')) m_current++;
  std::string str(m_start, m_current - m_start);
  if (str.find('x') != std::string::npos || str.find('b') != std::string::npos) {
    if (str[0] != '0') {
      asmError("Invalid number");
    } else {
      if (str.size() > 3 && (str.substr(3).find('x') != std::string::npos || str.substr(3).find('b') != std::string::npos)) {
        asmError("Invalid number");
      }
    }
  }
  return Token(TokenType::NUMBER, str, m_line);
}

xvm::Token xvm::Assembler::string() {
  m_start = m_current;
  if (*m_current == '"') m_current++;
  m_start = m_current;
  while (!isAtEnd() && *m_current != '"') m_current++;
  std::string str(m_start, m_current-m_start), result;
  for (size_t i = 0; i < str.size(); i++) {
    if (str[i] == '\\') {
      switch (str[++i]) {
        case '\\':
          result.push_back('\\');
          break;
        case 'n':
          result.push_back('\n');
          break;
        case 't':
          result.push_back('\t');
          break;
        case 'r':
          result.push_back('\r');
          break;
        default:
          result.push_back('\\');
          result.push_back(str[i]);
          break;
      }
    } else {
      result.push_back(str[i]);
    }
  }
  return Token(TokenType::STRING, result, m_line);
}

xvm::Token xvm::Assembler::getNextToken() {
  if (m_tokens[m_index+1].type == TokenType::IDENTIFIER) {
    auto itr = m_defines.find(m_tokens[m_index+1].str);
    if (itr != m_defines.end()) {
      int line = m_tokens[m_index].line;
      m_tokens.erase(m_tokens.begin() + m_index + 1);
      m_tokens.insert(m_tokens.begin() + m_index + 1, itr->second.begin(), itr->second.end());
      for (int i = 0; i < itr->second.size(); i++) { // Change line in replaced tokens
        m_tokens[m_index + i + 1].line = line;
      }
    }
  }
  return m_tokens[++m_index];
}

void xvm::Assembler::tokenize() {
  m_current = m_start;
  while (!isAtEnd() && !m_hadError) {
    m_start = m_current;
    switch (*m_current) {
      case ' ':
      case '\t': {
        skipWhitespace();
        break;
      }
      case '\n': {
        m_line++;
        m_current++;
        m_start = m_current;
        break;
      }
      case '"': {
        m_tokens.push_back(string());
        m_current++;
        break;
      }
      case '%': {
        m_tokens.push_back(Token(TokenType::PERCENT, m_start, 1, m_line));
        m_current++;
        break;
      }
      case ':': {
        m_tokens.push_back(Token(TokenType::COLON, m_start, 1, m_line));
        m_current++;
        break;
      }
      case ';': {
        while (!isAtEnd() && *m_current != '\n') m_current++;
        m_line++;
        break;
      }
      case '.': {
        m_tokens.push_back(Token(TokenType::DOT, m_start, 1, m_line));
        m_current++;
        break;
      }
      case '@': {
        m_tokens.push_back(Token(TokenType::AT, m_start, 1, m_line));
        m_current++;
        break;
      }
      case '+': {
        m_tokens.push_back(Token(TokenType::PLUS, m_start, 1, m_line));
        m_current++;
        break;
      }
      case '-': {
        m_tokens.push_back(Token(TokenType::MINUS, m_start, 1, m_line));
        m_current++;
        break;
      }
      case '\'': {
        m_tokens.push_back(character());
        m_current++;
        break;
      }
      default: {
        if (isalpha(*m_current)) {
          m_tokens.push_back(identifier());
        } else if (isdigit(*m_current)) {
          m_tokens.push_back(number());
        } else {
          asmError("Unexpected character: '%c'", *m_current);
          return;
        }
        break;
      }
    }
  }
  m_tokens.push_back(Token(TokenType::IDENTIFIER, g_eof, m_tokens.back().line+1));
}

void xvm::Assembler::pushOpcode(abi::AddressingMode mode, abi::OpCode opcode) {
  m_code.push_back(abi::encodeInstruction(mode, opcode));
}

void xvm::Assembler::pushByte(uint8_t value) {
  m_code.push_back(value);
}

void xvm::Assembler::pushInt16(int16_t value) {
  abi::N32 number;
  number.i16[0] = value;
  m_code.push_back(number.u8[0]);
  m_code.push_back(number.u8[1]);
}

void xvm::Assembler::pushInt32(int32_t value) {
  abi::N32 number;
  number.i32 = value;
  m_code.push_back(number.u8[0]);
  m_code.push_back(number.u8[1]);
  m_code.push_back(number.u8[2]);
  m_code.push_back(number.u8[3]);
}

int32_t xvm::Assembler::getAddress() {
  Token token = getNextToken();
  if (token.type == TokenType::NUMBER) {
    return token.toNumber();
  } else if (token.type == TokenType::MINUS) {
    if (isNextTokenOnSameLine()) {
      m_tokens.erase(m_tokens.begin() + m_index);
      m_tokens[m_index].str.insert(m_tokens[m_index].str.begin(), '-');
      return m_tokens[m_index].toNumber();
    }
  } else if (token.type == TokenType::IDENTIFIER) {
    m_labels[token.str].mentions.push_back({static_cast<int32_t>(m_code.size())});
    int32_t addressOffset = 0;
    Token nextToken = m_tokens[m_index+1];
    if (nextToken.type == TokenType::PLUS) {
      m_index++;
      addressOffset += getAddress();
    } else if (nextToken.type == TokenType::MINUS) {
      m_index++;
      addressOffset -= getAddress();
    }
    return addressOffset;
  } else {
    asmError(token, "Expected address or label");
  }
  return 0;
}

void xvm::Assembler::patchLabels() { // TODO: check for unpatched labels
  for (auto& label : m_labels) {
    for (auto mention : label.second.mentions) {
      abi::N32 value;
      value.i32 = label.second.address;
      m_code[mention.address]   += value.u8[0];
      m_code[mention.address+1] += value.u8[1];
      m_code[mention.address+2] += value.u8[2];
      m_code[mention.address+3] += value.u8[3];
    }
  }
}

void xvm::Assembler::parse() {
  using namespace abi;

  for (m_index = 0; m_index < m_tokens.size()-1; m_index++) {
    if (m_tokens[m_index].type == TokenType::IDENTIFIER) {
      if (m_tokens[m_index] == "halt") {
        pushOpcode(STK, HALT);
      } else if (m_tokens[m_index] == "push") {
        pushOpcode(IMM1, PUSH);
        pushInt32(getAddress());
      } else if (m_tokens[m_index] == "pop") {
        pushOpcode(STK, POP);
      } else if (m_tokens[m_index] == "dup") {
        pushOpcode(STK, DUP);
      } else if (m_tokens[m_index] == "rol") {
        pushOpcode(STK, ROL);
      } else if (m_tokens[m_index] == "rol3") {
        pushOpcode(STK, ROL3);
      } else if (m_tokens[m_index] == "deref8") {
        if (isNextTokenOnSameLine()) {
          pushOpcode(IMM1, DEREF8);
          pushInt32(getAddress());
        } else {
          pushOpcode(STK, DEREF8);
        }
      } else if (m_tokens[m_index] == "deref16") {
        if (isNextTokenOnSameLine()) {
          pushOpcode(IMM1, DEREF16);
          pushInt32(getAddress());
        } else {
          pushOpcode(STK, DEREF16);
        }
      } else if (m_tokens[m_index] == "deref32") {
        if (isNextTokenOnSameLine()) {
          pushOpcode(IMM1, DEREF32);
          pushInt32(getAddress());
        } else {
          pushOpcode(STK, DEREF32);
        }
      } else if (m_tokens[m_index] == "store8") {
        if (isNextTokenOnSameLine()) {
          pushOpcode(IMM1, STORE8);
          pushInt32(getAddress());
        } else {
          pushOpcode(STK, STORE8);
        }
      } else if (m_tokens[m_index] == "store16") {
        if (isNextTokenOnSameLine()) {
          pushOpcode(IMM1, STORE16);
          pushInt32(getAddress());
        } else {
          pushOpcode(STK, STORE16);
        }
      } else if (m_tokens[m_index] == "store32") {
        if (isNextTokenOnSameLine()) {
          pushOpcode(IMM1, STORE32);
          pushInt32(getAddress());
        } else {
          pushOpcode(STK, STORE32);
        }
      } else if (m_tokens[m_index] == "load8") {
        if (isNextTokenOnSameLine()) {
          pushOpcode(IMM1, LOAD8);
          pushInt32(getAddress());
        } else {
          pushOpcode(STK, LOAD8);
        }
      } else if (m_tokens[m_index] == "load16") {
        if (isNextTokenOnSameLine()) {
          pushOpcode(IMM1, LOAD16);
          pushInt32(getAddress());
        } else {
          pushOpcode(STK, LOAD16);
        }
      } else if (m_tokens[m_index] == "load32") {
        if (isNextTokenOnSameLine()) {
          pushOpcode(IMM1, LOAD32);
          pushInt32(getAddress());
        } else {
          pushOpcode(STK, LOAD32);
        }
      } else if (m_tokens[m_index] == "add") {
        if (isNextTokenOnSameLine()) {
          int32_t op1 = getAddress();
          if (isNextTokenOnSameLine()) {
            int32_t op2 = getAddress();
            pushOpcode(IMM2, ADD);
            pushInt32(op1);
            pushInt32(op2);
          } else {
            pushOpcode(IMM1, ADD);
            pushInt32(op1);
          }
        } else {
          pushOpcode(STK, ADD);
        }
      } else if (m_tokens[m_index] == "sub") {
        if (isNextTokenOnSameLine()) {
          int32_t op1 = getAddress();
          if (isNextTokenOnSameLine()) {
            int32_t op2 = getAddress();
            pushOpcode(IMM2, SUB);
            pushInt32(op1);
            pushInt32(op2);
          } else {
            pushOpcode(IMM1, SUB);
            pushInt32(op1);
          }
        } else {
          pushOpcode(STK, SUB);
        }
      } else if (m_tokens[m_index] == "mul") {
        if (isNextTokenOnSameLine()) {
          int32_t op1 = getAddress();
          if (isNextTokenOnSameLine()) {
            int32_t op2 = getAddress();
            pushOpcode(IMM2, MUL);
            pushInt32(op1);
            pushInt32(op2);
          } else {
            pushOpcode(IMM1, MUL);
            pushInt32(op1);
          }
        } else {
          pushOpcode(STK, MUL);
        }
      } else if (m_tokens[m_index] == "div") {
        if (isNextTokenOnSameLine()) {
          int32_t op1 = getAddress();
          if (isNextTokenOnSameLine()) {
            int32_t op2 = getAddress();
            pushOpcode(IMM2, DIV);
            pushInt32(op1);
            pushInt32(op2);
          } else {
            pushOpcode(IMM1, DIV);
            pushInt32(op1);
          }
        } else {
          pushOpcode(STK, DIV);
        }
      } else if (m_tokens[m_index] == "equ") {
        if (isNextTokenOnSameLine()) {
          int32_t op1 = getAddress();
          if (isNextTokenOnSameLine()) {
            int32_t op2 = getAddress();
            pushOpcode(IMM2, EQU);
            pushInt32(op1);
            pushInt32(op2);
          } else {
            pushOpcode(IMM1, EQU);
            pushInt32(op1);
          }
        } else {
          pushOpcode(STK, EQU);
        }
      } else if (m_tokens[m_index] == "gt") {
        if (isNextTokenOnSameLine()) {
          int32_t op1 = getAddress();
          if (isNextTokenOnSameLine()) {
            int32_t op2 = getAddress();
            pushOpcode(IMM2, GT);
            pushInt32(op1);
            pushInt32(op2);
          } else {
            pushOpcode(IMM1, GT);
            pushInt32(op1);
          }
        } else {
          pushOpcode(STK, GT);
        }
      } else if (m_tokens[m_index] == "lt") {
        if (isNextTokenOnSameLine()) {
          int32_t op1 = getAddress();
          if (isNextTokenOnSameLine()) {
            int32_t op2 = getAddress();
            pushOpcode(IMM2, LT);
            pushInt32(op1);
            pushInt32(op2);
          } else {
            pushOpcode(IMM1, LT);
            pushInt32(op1);
          }
        } else {
          pushOpcode(STK, LT);
        }
      } else if (m_tokens[m_index] == "dec") {
        if (isNextTokenOnSameLine()) {
          pushOpcode(IND, DEC);
          pushInt32(getAddress());
        } else {
          pushOpcode(STK, DEC);
        }
      } else if (m_tokens[m_index] == "inc") {
        if (isNextTokenOnSameLine()) {
          pushOpcode(IND, INC);
          pushInt32(getAddress());
        } else {
          pushOpcode(STK, INC);
        }
      } else if (m_tokens[m_index] == "shl") {
        if (!isNextTokenOnSameLine()) {
          asmError("Expected shift number");
          return;
        }
        pushOpcode(STK, SHL);
        pushInt32(getAddress()); // TODO: getInt
      } else if (m_tokens[m_index] == "shr") {
        if (!isNextTokenOnSameLine()) {
          asmError("Expected shift number");
          return;
        }
        pushOpcode(STK, SHR);
        pushInt32(getAddress()); // TODO: getInt
      } else if (m_tokens[m_index] == "and") {
        if (isNextTokenOnSameLine()) {
          int32_t op1 = getAddress();
          if (isNextTokenOnSameLine()) {
            int32_t op2 = getAddress();
            pushOpcode(IMM2, AND);
            pushInt32(op1);
            pushInt32(op2);
          } else {
            pushOpcode(IMM1, AND);
            pushInt32(op1);
          }
        } else {
          pushOpcode(STK, AND);
        }
      } else if (m_tokens[m_index] == "or") {
        if (isNextTokenOnSameLine()) {
          int32_t op1 = getAddress();
          if (isNextTokenOnSameLine()) {
            int32_t op2 = getAddress();
            pushOpcode(IMM2, OR);
            pushInt32(op1);
            pushInt32(op2);
          } else {
            pushOpcode(IMM1, OR);
            pushInt32(op1);
          }
        } else {
          pushOpcode(STK, OR);
        }
      } else if (m_tokens[m_index] == "jump") {
        if (isNextTokenOnSameLine()) {
          pushOpcode(IMM1, JUMP);
          pushInt32(getAddress());
        } else {
          pushOpcode(STK, JUMP);
        }
      } else if (m_tokens[m_index] == "jumpt") {
        if (isNextTokenOnSameLine()) {
          pushOpcode(IMM1, JUMPT);
          pushInt32(getAddress());
        } else {
          pushOpcode(STK, JUMPT);
        }
      } else if (m_tokens[m_index] == "jumpf") {
        if (isNextTokenOnSameLine()) {
          pushOpcode(IMM1, JUMPF);
          pushInt32(getAddress());
        } else {
          pushOpcode(STK, JUMPF);
        }
      } else if (m_tokens[m_index] == "call") {
        if (isNextTokenOnSameLine()) {
          pushOpcode(IMM1, CALL);
          pushInt32(getAddress());
        } else {
          pushOpcode(STK, CALL);
        }
      } else if (m_tokens[m_index] == "syscall") {
        if (isNextTokenOnSameLine()) {
          // pushInt32(getAddress()); // TODO: getInt
          pushOpcode(IMM1, SYSCALL);
          Token token = m_tokens[++m_index];
          if (token.type == TokenType::NUMBER) {
            pushInt32(token.toNumber());
          } else if (token.type == TokenType::IDENTIFIER) {
            if (m_syscalls.find(token.str) != m_syscalls.end()) {
              pushInt32(m_syscalls[token.str]);
            } else {
              asmError("No syscall definition for '%.*s' found", token.str.size(), token.str.data());
            }
          } else {
            asmError("Expecter number or identifier for syscall");
            return;
          }
        } else {
          pushOpcode(STK, SYSCALL);
        }
      } else if (m_tokens[m_index] == "ret") {
        pushOpcode(STK, RET);
      } else {
        if (m_tokens[m_index+1].type == TokenType::COLON) {
          m_labels[m_tokens[m_index].str].address = m_code.size();
          m_index++;
        } else {
          asmError(m_tokens[m_index], "Unexpected identifier: '%.*s'", m_tokens[m_index].str.size(), m_tokens[m_index].str.data());
        }
      }
    } else if (m_tokens[m_index].type == TokenType::PERCENT) {
      if (m_tokens[m_index+1].type == TokenType::IDENTIFIER) {
        m_index++;
        if (m_tokens[m_index] == "def") {
          /*
            Note on auto-dereferencing ('$' label)
            derefX STK instruction = 1 byte
             push $x
             ...
            becomes
             push x
             nop
             ...
            in patchLabels:
             push x
          -> deref8 <STK>
          */
          if (!isNextTokenOnSameLine()) {
            asmError("Expected variable type");
            return;
          }
          Token type = getNextToken();
          if (!isNextTokenOnSameLine()) {
            asmError("Expected variable name");
            return;
          }
          Token name = getNextToken();
          if (!isNextTokenOnSameLine()) {
            asmError("Expected variable value");
            return;
          }
          Token value = getNextToken();
          m_labels[name.str].address = m_code.size();
          if (type.str == "i8") {
            pushByte(value.toNumber());
          } else if (type.str == "i16") {
            pushInt16(value.toNumber());
          } else if (type.str == "i32") {
            pushInt32(value.toNumber());
          } else if (type.str == "str") {
            for (size_t i = 0; i < value.str.size(); i++) {
              pushByte(value.str[i]);
            }
            pushByte(0);
          } else {
            asmError("Unknown type '%.*s'", type.str.size(), type.str.data());
            return;
          }
        } else if (m_tokens[m_index] == "syscall") {
          if (!isNextTokenOnSameLine()) {
            asmError("Expected syscall name");
            return;
          }
          Token name = getNextToken();
          if (!isNextTokenOnSameLine()) {
            asmError("Expected syscall number");
            return;
          }
          Token number = getNextToken();
          m_syscalls[name.str] = number.toNumber();
        } else if (m_tokens[m_index] == "include") {
          if (!isNextTokenOnSameLine()) {
            asmError("Expected include file");
            return;
          }
          Token file = getNextToken();
          if (isFileExists(std::string(file.str))) {
            includeFile(std::string(file.str));
          } else {
            for (std::string& folder : m_includeFolders) {
              if (isFileExists(folder + std::string(file.str))) {
                includeFile(folder + std::string(file.str));
                break;
              }
            }
          }
        } else if (m_tokens[m_index] == "define") {
          define();
        } else if (m_tokens[m_index] == "undef") {
          undef();
        } else if (m_tokens[m_index] == "ifdef") {
          XVM_IFDEF_TEMPLATE("ifdef", ==);
        } else if (m_tokens[m_index] == "ifndef") {
          XVM_IFDEF_TEMPLATE("ifndef", !=);
        } else if (m_tokens[m_index] == "repeat") {
          if (!isNextTokenOnSameLine()) {
            asmError("Expected type");
            return;
          }
          Token type = getNextToken();
          if (!isNextTokenOnSameLine()) {
            asmError("Expected value");
            return;
          }
          Token value = getNextToken();
          if (!isNextTokenOnSameLine()) {
            asmError("Expected count");
            return;
          }
          Token count = getNextToken();
          int val = value.toNumber();
          int cnt = count.toNumber();
          for (int i = 0; i < cnt; i++) {
            if (type.str == "i8") {
              pushByte(val);
            } else if (type.str == "i16") {
              pushInt16(val);
            } else if (type.str == "i32") {
              pushInt32(val);
            } else {
              error("Unknown type '%s'", type.str.c_str());
              return;
            }
          }
        } else if (m_tokens[m_index] == "repeat_until") {
          if (!isNextTokenOnSameLine()) {
            asmError("Expected type");
            return;
          }
          Token type = getNextToken();
          if (!isNextTokenOnSameLine()) {
            asmError("Expected value");
            return;
          }
          Token value = getNextToken();
          if (!isNextTokenOnSameLine()) {
            asmError("Expected until");
            return;
          }
          Token until = getNextToken();
          int val = value.toNumber();
          int cnt = until.toNumber();
          while (m_code.size() < cnt) {
            if (type.str == "i8") {
              pushByte(val);
            } else if (type.str == "i16") {
              pushInt16(val);
            } else if (type.str == "i32") {
              pushInt32(val);
            } else {
              error("Unknown type '%s'", type.str.c_str());
              return;
            }
          }
        } else if (m_tokens[m_index] == "data") {
          if (!isNextTokenOnSameLine()) {
            asmError("Expected type");
            return;
          }
          Token type = getNextToken();
          if (!isNextTokenOnSameLine()) {
            asmError("Expected name");
            return;
          }
          Token name = getNextToken();
          m_labels[name.str].address = m_code.size();
          while (isNextTokenOnSameLine()) {
            int val = getNextToken().toNumber();
            if (type.str == "i8") {
              pushByte(val);
            } else if (type.str == "i16") {
              pushInt16(val);
            } else if (type.str == "i32") {
              pushInt32(val);
            } else {
              error("Unknown type '%s'", type.str.c_str());
              return;
            }
          }
        }

      }
    }
  }
}
