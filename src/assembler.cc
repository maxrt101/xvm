#include <xvm/assembler.h>
#include <xvm/version.h>
#include <xvm/config.h>
#include <xvm/utils.h>
#include <xvm/log.h>
#include <xvm/abi.h>

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

int xvm::Variable::size() const {
  switch (type) {
    case Type::I8:  return count;
    case Type::I16: return count * 2;
    case Type::I32: return count * 4;
    case Type::STR: return count + 1;
    default:        return count;
  }
}

xvm::Variable::Type xvm::Variable::stringToType(const std::string& type) {
  if (type == "i8") {
    return Type::I8;
  } else if (type == "i16") {
    return Type::I16;
  } else if (type == "i32") {
    return Type::I32;
  } else if (type == "str") {
    return Type::STR;
  } 
  return Type::I8;
}

std::string xvm::Variable::typeToString(Type type) {
  switch (type) {
    case Type::I8:  return "i8";
    case Type::I16: return "i16";
    case Type::I32: return "i32";
    case Type::STR: return "str";
    default:        return "<error>";
  }
}

xvm::Assembler::Assembler(const std::string& filename, const std::string& source) : m_filename(filename), m_source(source) {
  m_start = m_source.c_str();
  m_current = m_start;
}

int xvm::Assembler::assemble(Executable& exe, bool includeSymbols) {
  tokenize();
  if (m_hadError) return -1;

  parse();
  if (m_hadError) return -1;

  patchLabels();
  if (m_hadError) return -1;

  patchVariables();
  if (m_hadError) return -1;

  exe.magic = XVM_MAGIC;
  exe.version = XVM_VERSION_CODE;

  exe.sections.push_back(Executable::Section(
    SectionType::CODE,
    "code",
    m_code,
    0
  ));

  SymbolTable table;
  RelocationTable relocations;

  for (auto& label : m_labels) {
    if (!m_exportAll && std::find(m_exported.begin(), m_exported.end(), label.first) == m_exported.end()) {
      continue;
    }

    int size = 0;
    u16 flags = 0;

    if (m_variables.find(label.first) != m_variables.end()) {
      flags |= (u16) SymbolFlags::VARIABLE;
      size = m_variables[label.first].size();
    }

    if (std::find(m_externs.begin(), m_externs.end(), label.first) != m_externs.end()) {
      flags |= (u16) SymbolFlags::EXTERN;

      relocations.relocations.push_back({
        label.first, {}
      });

      for (auto& mention : label.second.mentions) {
        relocations.relocations.back().mentions.push_back({mention.address, mention.argumentNumber});
      }
    }

    table.addSymbol(
      label.second.address,
      label.first,
      flags | (u16)(label.second.isProcedure ? SymbolFlags::PROCEDURE : SymbolFlags::LABEL),
      size
    );
  }

  if (includeSymbols) {
    std::sort(table.symbols.begin(), table.symbols.end(),
      [](auto& lhs, auto& rhs) { return lhs.address < rhs.address; });

    exe.sections.push_back(table.toSection());
  }

  exe.sections.push_back(relocations.toSection());

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
  fprintf(stderr, "%s:%d %sError%s near '%.*s': ", m_filename.c_str(), t.line, colors::RED, colors::RESET, static_cast<int>(t.str.size()), t.str.data());
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
  while (isdigit(*m_current) || *m_current == 'x' || (*m_current >= 'a' && *m_current <= 'f')) m_current++;
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
    if (isDefined(m_tokens[m_index+1].str)) {
      int line = m_tokens[m_index].line;
      auto defined = getDefined(m_tokens[m_index+1].str);
      m_tokens.erase(m_tokens.begin() + m_index + 1);
      m_tokens.insert(m_tokens.begin() + m_index + 1, defined.begin(), defined.end());
      for (int i = 0; i < defined.size(); i++) { // Change line in replaced tokens
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
      case '$': {
        m_tokens.push_back(Token(TokenType::DOLLAR, m_start, 1, m_line));
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
      case '*': {
        m_tokens.push_back(Token(TokenType::STAR, m_start, 1, m_line));
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

void xvm::Assembler::pushOpcode(abi::OpCode opcode, abi::AddressingMode mode1, abi::AddressingMode mode2) {
  pushByte(abi::encodeFlags(mode1, mode2));
  pushByte(opcode);
}

void xvm::Assembler::pushByte(uint8_t value) {
  m_code.push_back(value);
}

void xvm::Assembler::pushInt16(int16_t value) {
  abi::N32 number;
  number._i16[0] = value;
  pushByte(number._u8[0]);
  pushByte(number._u8[1]);
}

void xvm::Assembler::pushInt32(int32_t value) {
  abi::N32 number;
  number._i32 = value;
  pushByte(number._u8[0]);
  pushByte(number._u8[1]);
  pushByte(number._u8[2]);
  pushByte(number._u8[3]);
}

int32_t xvm::Assembler::getAddress(u8 argumentNumber) {
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
    m_labels[token.str].mentions.push_back({static_cast<int32_t>(m_code.size()), argumentNumber});
    int32_t addressOffset = 0;
    Token nextToken = m_tokens[m_index+1];
    if (nextToken.type == TokenType::PLUS) {
      m_index++;
      addressOffset += getAddress(0);
    } else if (nextToken.type == TokenType::MINUS) {
      m_index++;
      addressOffset -= getAddress(0);
    }
    return addressOffset;
  } else {
    asmError(token, "Expected address or label");
  }
  return 0;
}

void xvm::Assembler::patchLabels() { // TODO: check for unpatched labels
  using namespace abi;

  for (auto& label : m_labels) {
    debug(2, "Label: '%s' at 0x%x", label.first.c_str(), label.second.address);
    for (auto mention : label.second.mentions) {
      if (label.second.address == -1 && std::find(m_externs.begin(), m_externs.end(), label.first) == m_externs.end()) {
        error("Unknown label: %s", label.first.c_str());
        m_hadError = true;
      }
      N32 value;
      if (config::asBool("pic")) {
        patchAddressingMode(label.second.address, mention.address, mention.argumentNumber);
        value._i32 = std::abs(label.second.address - mention.address);
      } else {
        value._i32 = label.second.address;
      }
      m_code[mention.address]   += value._u8[0];
      m_code[mention.address+1] += value._u8[1];
      m_code[mention.address+2] += value._u8[2];
      m_code[mention.address+3] += value._u8[3];
      debug(2, "Label '%s' mention at 0x%x patched", label.first.c_str(), mention.address);
    }
  }
}

void xvm::Assembler::patchVariables() {
  using namespace abi;

  for (auto& var : m_variables) {
    debug(2, "Variable: '%s' type '%s' at 0x%x", var.first.c_str(), Variable::typeToString(var.second.type).c_str(), var.second.address);
    for (auto mention : var.second.mentions) {
      if (var.second.address == -1) {
        error("Unknown variable: %s", var.first.c_str());
        m_hadError = true;
      }
      N32 value;
      if (mention.isDeref) {
        OpCode op;
        switch (var.second.type) {
          case Variable::Type::STR:
          case Variable::Type::I8: {
            op = DEREF8;
            break;
          }
          case Variable::Type::I16: {
            op = DEREF16;
            break;
          }
          case Variable::Type::I32: {
            op = DEREF32;
            break;
          }
          default: {
            asmError("Invalid var type (%d)", var.second.type);
            return;
          }
        }
        m_code[mention.address]   = encodeFlags(STK, _NONE); //encodeInstruction(STK, _NONE, op);
        m_code[mention.address+1] = op; 
        debug(2, "Variable '%s' deref mention at 0x%x patched with %s", var.first.c_str(), mention.address, opCodeToString(op).c_str());
      } else {
        if (config::asBool("pic")) {
          patchAddressingMode(var.second.address, mention.address, mention.argumentNumber);
          value._i32 = std::abs(var.second.address - mention.address);
        } else {
          value._i32 = var.second.address;
        }
        m_code[mention.address]   += value._u8[0];
        m_code[mention.address+1] += value._u8[1];
        m_code[mention.address+2] += value._u8[2];
        m_code[mention.address+3] += value._u8[3];
        debug(2, "Variable '%s' mention at 0x%x patched", var.first.c_str());
      }
    }
  }
}

void xvm::Assembler::patchAddressingMode(i32 labelAddress, i32 mentionAddress, u8 argumentNumber) {
  using namespace abi;
  if (labelAddress - mentionAddress < 0) {
    // NRO
    if (argumentNumber == 1) {
      auto mode2 = extractModeArg2(m_code[mentionAddress - 2]);
      m_code[mentionAddress - 2] = encodeFlags(NRO, mode2);
    } else if (argumentNumber == 2) {
      auto mode1 = extractModeArg1(m_code[mentionAddress - 6]);
      m_code[mentionAddress - 6] = encodeFlags(mode1, NRO);
    }
  } else {
    // PRO
    if (argumentNumber == 1) {
      auto mode2 = extractModeArg2(m_code[mentionAddress - 2]);
      m_code[mentionAddress - 2] = encodeFlags(PRO, mode2);
    } else if (argumentNumber == 2) {
      auto mode1 = extractModeArg1(m_code[mentionAddress - 6]);
      m_code[mentionAddress - 6] = encodeFlags(mode1, PRO);
    }
  }
}

void xvm::Assembler::parse() {
  using namespace abi;

  for (m_index = 0; m_index < m_tokens.size()-1; m_index++) {
    if (m_tokens[m_index].type == TokenType::IDENTIFIER) {
      if (m_tokens[m_index] == "halt") {
        pushOpcode(HALT, _NONE, _NONE);
      } else if (m_tokens[m_index] == "push") {
        pushOpcode(PUSH, IMM);
        if (m_tokens[m_index+1].type == TokenType::DOLLAR) {
          m_index++;
          if (m_tokens[m_index+1].type != TokenType::IDENTIFIER) {
            asmError(m_tokens[m_index+1], "Expected label after '$'");
          }
          pushInt32(getAddress());
          auto varname = m_tokens[m_index].str;
          if (m_variables.find(varname) == m_variables.end()) {
            m_variables[varname] = {};
          }
          m_variables[varname].mentions.push_back({static_cast<int32_t>(m_code.size()), 1, true});
          pushByte(0);
          pushByte(NOP);
        } else {
          pushInt32(getAddress());
        }
      } else if (m_tokens[m_index] == "pop") {
        if (isNextTokenOnSameLine()) {
          pushOpcode(POP, IMM);
          pushInt32(getAddress());
        } else {
          pushOpcode(POP, _NONE);
        }
      } else if (m_tokens[m_index] == "dup") {
        pushOpcode(DUP, _NONE);
      } else if (m_tokens[m_index] == "rol") {
        pushOpcode(ROL, _NONE);
      } else if (m_tokens[m_index] == "rol3") {
        pushOpcode(ROL3, _NONE);
      } else if (m_tokens[m_index] == "deref8") {
        if (isNextTokenOnSameLine()) {
          pushOpcode(DEREF8, IMM);
          pushInt32(getAddress());
        } else {
          pushOpcode(DEREF8, STK);
        }
      } else if (m_tokens[m_index] == "deref16") {
        if (isNextTokenOnSameLine()) {
          pushOpcode(DEREF16, IMM);
          pushInt32(getAddress());
        } else {
          pushOpcode(DEREF16, STK);
        }
      } else if (m_tokens[m_index] == "deref32") {
        if (isNextTokenOnSameLine()) {
          pushOpcode(DEREF32, IMM);
          pushInt32(getAddress());
        } else {
          pushOpcode(DEREF32, STK);
        }
      } else if (m_tokens[m_index] == "store8") {
        if (isNextTokenOnSameLine()) {
          if (isNextTokenOnSameLine()) {
            pushOpcode(STORE8, IMM, IMM);
            pushInt32(getAddress());
            pushInt32(getAddress());
          } else {
            pushOpcode(STORE8, IMM, STK);
            pushInt32(getAddress());
          }
        } else {
          pushOpcode(STORE8, STK, STK);
        }
      } else if (m_tokens[m_index] == "store16") {
        if (isNextTokenOnSameLine()) {
          if (isNextTokenOnSameLine()) {
            pushOpcode(STORE16, IMM, IMM);
            pushInt32(getAddress());
            pushInt32(getAddress());
          } else {
            pushOpcode(STORE16, IMM, STK);
            pushInt32(getAddress());
          }
        } else {
          pushOpcode(STORE16, STK, STK);
        }
      } else if (m_tokens[m_index] == "store32") {
        if (isNextTokenOnSameLine()) {
          if (isNextTokenOnSameLine()) {
            pushOpcode(STORE32, IMM, IMM);
            pushInt32(getAddress());
            pushInt32(getAddress());
          } else {
            pushOpcode(STORE32, IMM, STK);
            pushInt32(getAddress());
          }
        } else {
          pushOpcode(STORE32, STK, STK);
        }
      } else if (m_tokens[m_index] == "load8") {
        if (isNextTokenOnSameLine()) {
          pushOpcode(LOAD8, IMM);
          pushInt32(getAddress());
        } else {
          pushOpcode(LOAD8, STK);
        }
      } else if (m_tokens[m_index] == "load16") {
        if (isNextTokenOnSameLine()) {
          pushOpcode(LOAD16, IMM);
          pushInt32(getAddress());
        } else {
          pushOpcode(LOAD16, STK);
        }
      } else if (m_tokens[m_index] == "load32") {
        if (isNextTokenOnSameLine()) {
          pushOpcode(LOAD32, IMM);
          pushInt32(getAddress());
        } else {
          pushOpcode(LOAD32, STK);
        }
      } else if (m_tokens[m_index] == "add") {
        if (isNextTokenOnSameLine()) {
          int32_t op1 = getAddress();
          if (isNextTokenOnSameLine()) {
            int32_t op2 = getAddress(2);
            pushOpcode(ADD, IMM, IMM);
            pushInt32(op1);
            pushInt32(op2);
          } else {
            pushOpcode(ADD, IMM, STK);
            pushInt32(op1);
          }
        } else {
          pushOpcode(ADD, STK, STK);
        }
      } else if (m_tokens[m_index] == "sub") {
        if (isNextTokenOnSameLine()) {
          int32_t op1 = getAddress();
          if (isNextTokenOnSameLine()) {
            int32_t op2 = getAddress(2);
            pushOpcode(SUB, IMM, IMM);
            pushInt32(op1);
            pushInt32(op2);
          } else {
            pushOpcode(SUB, IMM, STK);
            pushInt32(op1);
          }
        } else {
          pushOpcode(SUB, STK, STK);
        }
      } else if (m_tokens[m_index] == "mul") {
        if (isNextTokenOnSameLine()) {
          int32_t op1 = getAddress();
          if (isNextTokenOnSameLine()) {
            int32_t op2 = getAddress(2);
            pushOpcode(MUL, IMM, IMM);
            pushInt32(op1);
            pushInt32(op2);
          } else {
            pushOpcode(MUL, IMM, STK);
            pushInt32(op1);
          }
        } else {
          pushOpcode(MUL, STK, STK);
        }
      } else if (m_tokens[m_index] == "div") {
        if (isNextTokenOnSameLine()) {
          int32_t op1 = getAddress();
          if (isNextTokenOnSameLine()) {
            int32_t op2 = getAddress(2);
            pushOpcode(DIV, IMM, IMM);
            pushInt32(op1);
            pushInt32(op2);
          } else {
            pushOpcode(DIV, IMM, STK);
            pushInt32(op1);
          }
        } else {
          pushOpcode(DIV, STK, STK);
        }
      } else if (m_tokens[m_index] == "equ") {
        if (isNextTokenOnSameLine()) {
          int32_t op1 = getAddress();
          if (isNextTokenOnSameLine()) {
            int32_t op2 = getAddress(2);
            pushOpcode(EQU, IMM, IMM);
            pushInt32(op1);
            pushInt32(op2);
          } else {
            pushOpcode(EQU, IMM, STK);
            pushInt32(op1);
          }
        } else {
          pushOpcode(EQU, STK, STK);
        }
      } else if (m_tokens[m_index] == "gt") {
        if (isNextTokenOnSameLine()) {
          int32_t op1 = getAddress();
          if (isNextTokenOnSameLine()) {
            int32_t op2 = getAddress(2);
            pushOpcode(GT, IMM, IMM);
            pushInt32(op1);
            pushInt32(op2);
          } else {
            pushOpcode(GT, IMM, STK);
            pushInt32(op1);
          }
        } else {
          pushOpcode(GT, STK, STK);
        }
      } else if (m_tokens[m_index] == "lt") {
        if (isNextTokenOnSameLine()) {
          int32_t op1 = getAddress();
          if (isNextTokenOnSameLine()) {
            int32_t op2 = getAddress(2);
            pushOpcode(LT, IMM, IMM);
            pushInt32(op1);
            pushInt32(op2);
          } else {
            pushOpcode(LT, IMM, STK);
            pushInt32(op1);
          }
        } else {
          pushOpcode(LT, STK, STK);
        }
      } else if (m_tokens[m_index] == "dec") {
        if (isNextTokenOnSameLine()) {
          pushOpcode(DEC, ABS); // FIXME:
          pushInt32(getAddress());
        } else {
          pushOpcode(DEC, STK);
        }
      } else if (m_tokens[m_index] == "inc") {
        if (isNextTokenOnSameLine()) {
          pushOpcode(INC, ABS); // FIXME:
          pushInt32(getAddress());
        } else {
          pushOpcode(INC, STK);
        }
      } else if (m_tokens[m_index] == "shl") {
        if (!isNextTokenOnSameLine()) {
          asmError("Expected shift number");
          return;
        }
        pushOpcode(SHL, STK);
        pushInt32(getAddress()); // TODO: getInt
      } else if (m_tokens[m_index] == "shr") {
        if (!isNextTokenOnSameLine()) {
          asmError("Expected shift number");
          return;
        }
        pushOpcode(SHR, STK);
        pushInt32(getAddress()); // TODO: getInt
      } else if (m_tokens[m_index] == "and") {
        if (isNextTokenOnSameLine()) {
          int32_t op1 = getAddress();
          if (isNextTokenOnSameLine()) {
            int32_t op2 = getAddress(2);
            pushOpcode(AND, IMM, IMM);
            pushInt32(op1);
            pushInt32(op2);
          } else {
            pushOpcode(AND, IMM, STK);
            pushInt32(op1);
          }
        } else {
          pushOpcode(AND, STK, STK);
        }
      } else if (m_tokens[m_index] == "or") {
        if (isNextTokenOnSameLine()) {
          int32_t op1 = getAddress();
          if (isNextTokenOnSameLine()) {
            int32_t op2 = getAddress(2);
            pushOpcode(OR, IMM, IMM);
            pushInt32(op1);
            pushInt32(op2);
          } else {
            pushOpcode(OR, IMM);
            pushInt32(op1);
          }
        } else {
          pushOpcode(OR, STK, STK);
        }
      } else if (m_tokens[m_index] == "jump") {
        if (isNextTokenOnSameLine()) {
          pushOpcode(JUMP, IMM);
          pushInt32(getAddress());
        } else {
          pushOpcode(JUMP, STK);
        }
      } else if (m_tokens[m_index] == "jumpt") {
        if (isNextTokenOnSameLine()) {
          pushOpcode(JUMPT, IMM);
          pushInt32(getAddress());
        } else {
          pushOpcode(JUMPT, STK);
        }
      } else if (m_tokens[m_index] == "jumpf") {
        if (isNextTokenOnSameLine()) {
          pushOpcode(JUMPF, IMM);
          pushInt32(getAddress());
        } else {
          pushOpcode(JUMPF, STK);
        }
      } else if (m_tokens[m_index] == "call") {
        if (isNextTokenOnSameLine()) {
          pushOpcode(CALL, IMM);
          if (m_tokens[m_index+1].type == TokenType::IDENTIFIER) {
            m_labels[m_tokens[m_index+1].str].isProcedure = true;
          }
          pushInt32(getAddress());
        } else {
          pushOpcode(CALL, STK);
        }
      } else if (m_tokens[m_index] == "syscall") {
        if (isNextTokenOnSameLine()) {
          // pushInt32(getAddress()); // TODO: getInt
          pushOpcode(SYSCALL, IMM);
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
          pushOpcode(SYSCALL, STK);
        }
      } else if (m_tokens[m_index] == "ret") {
        pushOpcode(RET, _NONE);
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
          int count = 1;
          if (type.str == "i8") {
            pushByte(value.toNumber());
          } else if (type.str == "i16") {
            pushInt16(value.toNumber());
          } else if (type.str == "i32") {
            pushInt32(value.toNumber());
          } else if (type.str == "str") {
            count = value.str.size();
            for (size_t i = 0; i < value.str.size(); i++) {
              pushByte(value.str[i]);
            }
            pushByte(0);
          } else {
            asmError("Unknown type '%.*s'", type.str.size(), type.str.data());
            return;
          }
          m_variables[name.str].name = name.str;
          m_variables[name.str].address = m_labels[name.str].address;
          m_variables[name.str].type = Variable::stringToType(type.str);
          m_variables[name.str].count = count;
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
          bool fileIncluded = false;
          if (isFileExists(std::string(file.str))) {
            includeFile(std::string(file.str));
            fileIncluded = true;
          } else {
            for (std::string& folder : m_includeFolders) {
              if (isFileExists(folder + std::string(file.str))) {
                includeFile(folder + std::string(file.str));
                fileIncluded = true;
                break;
              }
            }
          }
          if (!fileIncluded) {
            asmError(file, "Can't include '%s': no such file found in include path", file.str.c_str());
            return;
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
          int count = 0;
          while (isNextTokenOnSameLine()) {
            count++;
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
          m_variables[name.str].name = name.str;
          m_variables[name.str].address = m_labels[name.str].address;
          m_variables[name.str].type = Variable::stringToType(type.str);
          m_variables[name.str].count = count;
        } else if (m_tokens[m_index] == "export") {
          while (isNextTokenOnSameLine()) {
            Token exportName = getNextToken();
            if (exportName.str == "*") {
              m_exportAll = true;
            } else {
              m_exported.push_back(exportName.str);
            }
          }
        } else if (m_tokens[m_index] == "unexport") {
          while (isNextTokenOnSameLine()) {
            Token exportName = getNextToken();
            if (exportName.str == "*") {
              m_exportAll = false;
            } else {
              auto itr = std::find(m_exported.begin(), m_exported.end(), exportName.str);
              if (itr != m_exported.end()) {
                m_exported.erase(itr);
              }
            }
          }
        } else if (m_tokens[m_index] == "extern") {
          while (isNextTokenOnSameLine()) {
            Token externName = getNextToken();
            m_externs.push_back(externName.str);
          }
        } else {
          asmError("Unknown directive '%s'", m_tokens[m_index].str.c_str());
          return;
        }

      }
    }
  }
}
