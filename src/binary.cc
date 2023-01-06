#include <xvm/binary.h>
#include <xvm/config.h>
#include <xvm/log.h>
#include <xvm/abi.h>

#include <cstdio>
#include <errno.h>

size_t xvm::Executable::Section::size() {
  return label.size() + data.size() + 12;
}

std::vector<uint8_t> xvm::Executable::Section::toBytes() {
  std::vector<uint8_t> buffer;
  abi::N32 n;

  auto push_u32 = [&n, &buffer] {
    for (int i = 0; i < 4; i++)
      buffer.push_back(n.u8[i]);
  };

  for (char c : label) {
    buffer.push_back(c);
  }
  buffer.push_back(0);

  n.u32 = (uint32_t) type;
  push_u32();

  n.u32 = data.size();
  push_u32();

  n.u32 = checksum;
  push_u32();

  buffer.insert(buffer.end(), data.begin(), data.end());

  return buffer;
}

xvm::Executable::Section xvm::Executable::Section::fromBuffer(const uint8_t* data, size_t offset) {
  Section section;

  for (char c = data[offset++]; c != '\0'; c = data[offset++]) {
    section.label.push_back(c);
  }

  abi::N32 n;
  readInt32(n, data, offset);
  section.type = (SectionType) n.u32;

  readInt32(n, data, offset += 4);
  uint32_t size = n.u32;

  readInt32(n, data, offset += 4);
  section.checksum = n.u32;

  offset += 4;

  section.data = std::vector(data + offset, data + offset + size);

  return section;
}

std::vector<std::string> xvm::Executable::Section::getStringLine() {
  return {label, sectionTypeToString(type), std::to_string(data.size())};
}

const std::vector<std::string> xvm::Executable::Section::getFieldNames() {
  return {"Label", "Type", "Size"};
}

bool xvm::Executable::SymbolTable::Symbol::isLabel() {
  return flags & (uint16_t) SymbolFlags::LABEL;
}

bool xvm::Executable::SymbolTable::Symbol::isProcedure() {
  return flags & (uint16_t) SymbolFlags::PROCEDURE;
}

bool xvm::Executable::SymbolTable::Symbol::isVariable() {
  return flags & (uint16_t) SymbolFlags::VARIABLE;
}

std::vector<std::string> xvm::Executable::SymbolTable::Symbol::getStringLine() {
  char buffer[32] = {0};
  snprintf(buffer, sizeof buffer, "0x%x", address);

  return {
    buffer,
    std::string(isLabel() ? "L" : "-") + (isProcedure() ? "P" : "-") + (isVariable() ? "V" : "-"),
    std::to_string(data_width),
    label
  };
}

const std::vector<std::string> xvm::Executable::SymbolTable::Symbol::getFieldNames() {
  return {"Addr", "Flags", "Size", "Label"};
}

void xvm::Executable::SymbolTable::addSymbol(uint32_t address, const std::string& label, uint16_t flags, uint16_t data_width) {
  symbols.push_back({address, flags, data_width, label});
}

xvm::Executable::SymbolTable::Symbol xvm::Executable::SymbolTable::getByAddress(uint32_t address) {
  for (auto& symbol : symbols) {
    if (symbol.address == address) {
      return symbol;
    }
  }
  return {.label = "", .address = 0};
}

xvm::Executable::SymbolTable::Symbol xvm::Executable::SymbolTable::getByLabel(const std::string& label) {
  for (auto& symbol : symbols) {
    if (symbol.label == label) {
      return symbol;
    }
  }
  return {.label = "", .address = 0};
}

xvm::Executable::Section xvm::Executable::SymbolTable::toSection(const std::string& label) {
  Section section;
  abi::N32 n;

  section.label = label;
  section.type = SectionType::SYMBOLS;
  section.checksum = 0;

  for (auto& symbol : symbols) {
    n.i32 = symbol.address;
    for (int i = 0; i < 4; i++)
      section.data.push_back(n.u8[i]);

    n.u16[0] = symbol.flags;
    section.data.push_back(n.u8[0]);
    section.data.push_back(n.u8[1]);

    n.u16[0] = symbol.data_width;
    section.data.push_back(n.u8[0]);
    section.data.push_back(n.u8[1]);

    for (int i = 0; i < symbol.label.size(); i++)
      section.data.push_back(symbol.label[i]);
    section.data.push_back(0);
  }

  return section;
}

xvm::Executable::SymbolTable xvm::Executable::SymbolTable::fromSection(Section& section) {
  SymbolTable table;
  abi::N32 n;

  for (int i = 0; i < section.data.size(); ) {
    Symbol symbol;

    readInt32(n, section.data.data(), i);
    symbol.address = n.u32;

    readInt16(n, section.data.data(), i += 4);
    symbol.flags = n.u16[0];

    readInt16(n, section.data.data(), i += 2);
    symbol.data_width = n.u16[0];

    i += 2;

    for (char c = section.data[i++]; c != '\0'; c = section.data[i++]) {
      symbol.label.push_back(c);
    }

    table.symbols.push_back(symbol);
  }

  return table;
}

bool xvm::Executable::hasSection(const std::string& label) {
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

std::vector<uint8_t> xvm::Executable::toBytes() {
  std::vector<uint8_t> buffer;
  abi::N32 n;

  auto push_u32 = [&n, &buffer] {
    for (int i = 0; i < 4; i++)
      buffer.push_back(n.u8[i]);
  };

  n.u32 = magic;
  push_u32();

  n.u32 = version;
  push_u32();

  n.u32 = flags;
  push_u32();

  n.u32 = sections.size();
  push_u32();

  for (auto& section : sections) {
    auto section_bytes = section.toBytes();
    buffer.insert(buffer.end(), section_bytes.begin(), section_bytes.end());
  }

  return buffer;
}

void xvm::Executable::toFile(const std::string& filename) {
  FILE* file = fopen(filename.c_str(), "wb");

  if (!file) {
    error("Failed to open/create '%s'", filename.c_str());
    return;
  }

  auto buffer = toBytes();
  fwrite(buffer.data(), 1, buffer.size(), file);

  fclose(file);
}

xvm::Executable xvm::Executable::fromBuffer(const uint8_t* data) {
  Executable exe;
  
  abi::N32 n;
  readInt32(n, data, 0);
  exe.magic = n.u32;

  readInt32(n, data, 4);
  exe.version = n.u32;

  readInt32(n, data, 8);
  exe.flags = n.u32;
  
  readInt32(n, data, 12);
  uint32_t section_count = n.u32;

  uint32_t offset = 16;

  for (uint32_t i = 0; i < section_count; i++) {
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

  uint8_t* data = new uint8_t[size];
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
