#include <xvm/executable.h>
#include <xvm/bytecode.h>
#include <xvm/config.h>
#include <xvm/log.h>
#include <xvm/abi.h>

#include <cstdio>
#include <cctype>
#include <errno.h>


xvm::Executable::Section::Section(SectionType type, std::string label, std::vector<u8> data, u32 addr)
  : magic(XVM_SECTION_MAGIC), type(type), addr(addr), checksum(0), label(label), data(data) {
}

size_t xvm::Executable::Section::size() const {
  return label.size() + data.size() + 12;
}

std::vector<u8> xvm::Executable::Section::toBytes() const {
  std::vector<u8> buffer;
  abi::N32 n;

  auto push_u32 = [&n, &buffer] {
    for (int i = 0; i < 4; i++)
      buffer.push_back(n._u8[i]);
  };

  for (char c : label) {
    buffer.push_back(c);
  }
  buffer.push_back(0);

  n._u32 = (u32) type;
  push_u32();

  n._u32 = data.size();
  push_u32();

  n._u32 = checksum;
  push_u32();

  buffer.insert(buffer.end(), data.begin(), data.end());

  return buffer;
}

xvm::Executable::Section xvm::Executable::Section::fromBuffer(const u8* data, size_t offset) {
  Section section;

  for (char c = data[offset++]; c != '\0'; c = data[offset++]) {
    section.label.push_back(c);
  }

  abi::N32 n;
  readInt32(n, data, offset);
  section.type = (SectionType) n._u32;

  readInt32(n, data, offset += 4);
  u32 size = n._u32;

  readInt32(n, data, offset += 4);
  section.checksum = n._u32;

  offset += 4;

  section.data = std::vector(data + offset, data + offset + size);

  return section;
}

std::vector<std::string> xvm::Executable::Section::getStringLine() const {
  return {label, sectionTypeToString(type), std::to_string(data.size())};
}

const std::vector<std::string> xvm::Executable::Section::getFieldNames() {
  return {"Label", "Type", "Size"};
}

bool xvm::SymbolTable::Symbol::isLabel() const {
  return flags & (u16) SymbolFlags::LABEL;
}

bool xvm::SymbolTable::Symbol::isProcedure() const {
  return flags & (u16) SymbolFlags::PROCEDURE;
}

bool xvm::SymbolTable::Symbol::isVariable() const {
  return flags & (u16) SymbolFlags::VARIABLE;
}

std::vector<std::string> xvm::SymbolTable::Symbol::getStringLine() const {
  char buffer[32] = {0};
  snprintf(buffer, sizeof buffer, "0x%x", address);

  return {
    buffer,
    std::string(isLabel() ? "L" : "-") + (isProcedure() ? "P" : "-") + (isVariable() ? "V" : "-"),
    std::to_string(size),
    label
  };
}

const std::vector<std::string> xvm::SymbolTable::Symbol::getFieldNames() {
  return {"Addr", "Flags", "Size", "Label"};
}

void xvm::SymbolTable::addSymbol(u32 address, const std::string& label, u16 flags, u16 size) {
  symbols.push_back({address, flags, size, label});
}

xvm::SymbolTable::Symbol& xvm::SymbolTable::getByAddress(u32 address) {
  for (auto& symbol : symbols) {
    if (symbol.address == address) {
      return symbol;
    }
  }
  throw "No such symbol";
}

xvm::SymbolTable::Symbol& xvm::SymbolTable::getByLabel(const std::string& label) {
  for (auto& symbol : symbols) {
    if (symbol.label == label) {
      return symbol;
    }
  }
  throw "No such symbol";
}

bool xvm::SymbolTable::hasAddress(u32 address) const {
  for (auto& symbol : symbols) {
    if (symbol.address == address) {
      return true;
    }
  }
  return false;
}

bool xvm::SymbolTable::hasLabel(const std::string& label) const {
  for (auto& symbol : symbols) {
    if (symbol.label == label) {
      return true;
    }
  }
  return false;
}

xvm::Executable::Section xvm::SymbolTable::toSection(const std::string& label) const {
  Executable::Section section;
  abi::N32 n;

  section.label = label;
  section.type = SectionType::SYMBOLS;
  section.checksum = 0;

  for (auto& symbol : symbols) {
    n._i32 = symbol.address;
    for (int i = 0; i < 4; i++)
      section.data.push_back(n._u8[i]);

    n._u16[0] = symbol.flags;
    section.data.push_back(n._u8[0]);
    section.data.push_back(n._u8[1]);

    n._u16[0] = symbol.size;
    section.data.push_back(n._u8[0]);
    section.data.push_back(n._u8[1]);

    for (int i = 0; i < symbol.label.size(); i++)
      section.data.push_back(symbol.label[i]);
    section.data.push_back(0);
  }

  return section;
}

xvm::SymbolTable xvm::SymbolTable::fromSection(const Executable::Section& section) {
  SymbolTable table;
  abi::N32 n;

  for (int i = 0; i < section.data.size(); ) {
    Symbol symbol;

    readInt32(n, section.data.data(), i);
    symbol.address = n._u32;

    readInt16(n, section.data.data(), i += 4);
    symbol.flags = n._u16[0];

    readInt16(n, section.data.data(), i += 2);
    symbol.size = n._u16[0];

    i += 2;

    for (char c = section.data[i++]; c != '\0'; c = section.data[i++]) {
      symbol.label.push_back(c);
    }

    table.symbols.push_back(symbol);
  }

  return table;
}

bool xvm::Executable::hasSection(const std::string& label) const {
  for (auto& section : sections) {
    if (section.label == label) {
      return true;
    }
  }
  return false;
}

xvm::Executable::Section& xvm::Executable::getSection(const std::string& label) {
  for (auto& section : sections) {
    if (section.label == label) {
      return section;
    }
  }
  throw "No such section";
}

const xvm::Executable::Section& xvm::Executable::getSection(const std::string& label) const {
  for (auto& section : sections) {
    if (section.label == label) {
      return section;
    }
  }
  throw "No such section";
}

void xvm::Executable::disassemble() const {
  if (!hasSection("code")) return;

  const auto& code = getSection("code");

  bool hasSymbols = hasSection("symbols");  

  SymbolTable table;

  if (hasSymbols) {
    table = SymbolTable::fromSection(getSection("symbols"));
  }

  for (int i = 0; i < code.data.size(); ) {
    if (hasSymbols && table.hasAddress(i)) {
      const auto& symbol = table.getByAddress(i);
      if (symbol.isVariable()) {
        printf("0x%04x | %s | ", i, symbol.label.c_str());
        char* buffer = new char[symbol.size+1] {0};
        for (int idx = 0; idx < symbol.size; i++) {
          printf("%02x ", code.data[i]);
          buffer[idx++] = isprint(code.data[i]) ? code.data[i] : '.';
        }
        printf("| %s\n", buffer);
        delete [] buffer;
        continue;
      }
    }

    i = xvm::abi::disassembleInstruction(code.data.data(), i);
  }
}

std::vector<u8> xvm::Executable::toBytes() const {
  std::vector<u8> buffer;
  abi::N32 n;

  auto push_u32 = [&n, &buffer] {
    for (int i = 0; i < 4; i++)
      buffer.push_back(n._u8[i]);
  };

  n._u32 = magic;
  push_u32();

  n._u32 = version;
  push_u32();

  n._u32 = flags;
  push_u32();

  n._u32 = sections.size();
  push_u32();

  for (auto& section : sections) {
    auto section_bytes = section.toBytes();
    buffer.insert(buffer.end(), section_bytes.begin(), section_bytes.end());
  }

  return buffer;
}

void xvm::Executable::toFile(const std::string& filename) const {
  FILE* file = fopen(filename.c_str(), "wb");

  if (!file) {
    error("Failed to open/create '%s'", filename.c_str());
    return;
  }

  auto buffer = toBytes();
  fwrite(buffer.data(), 1, buffer.size(), file);

  fclose(file);
}

xvm::Executable xvm::Executable::fromBuffer(const u8* data) {
  Executable exe;
  
  abi::N32 n;
  readInt32(n, data, 0);
  exe.magic = n._u32;

  readInt32(n, data, 4);
  exe.version = n._u32;

  readInt32(n, data, 8);
  exe.flags = n._u32;
  
  readInt32(n, data, 12);
  u32 section_count = n._u32;

  u32 offset = 16;

  for (u32 i = 0; i < section_count; i++) {
    exe.sections.push_back(Section::fromBuffer(data, offset));
    offset += exe.sections.back().size() + 1;
  }

  return exe;
}

xvm::Executable xvm::Executable::fromFile(const std::string& filename) {
  Executable exe;
  FILE* file = fopen(filename.c_str(), "rb");
  if (!file) {
    if (errno == 2) {
      error("No such file or directory: '%s'", filename.c_str());
      return exe;
    } else {
      error("Failed to open '%s'", filename.c_str());
      return exe;
    }
  }

  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  rewind(file);

  u8* data = new u8[size];
  if (fread(data, 1, size, file) != size) {
    error("Error: couldn't read file (errno=%d)", errno);
    delete [] data;
    fclose(file);
    return exe;
  }

  fclose(file);

  exe = Executable::fromBuffer(data);

  delete [] data;
  return exe;
}

std::string xvm::sectionTypeToString(SectionType type) {
  switch (type) {
    case SectionType::CODE:    return "code";
    case SectionType::DATA:    return "data";
    case SectionType::SYMBOLS: return "symbols";
    default:                   return "<error>";
  }
}
