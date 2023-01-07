#ifndef _XVM_BINARY_H_
#define _XVM_BINARY_H_ 1

#include <cstdint>
#include <string>
#include <vector>

#define XVM_MAGIC 0xdeadbeef
#define XVM_BAD_MAGIC -1U

namespace xvm {

/*
Executable format:
0-4     u32  MAGIC
4-8     u32  VERSION
8-12    u32  FLAGS
12-16   u32  SECTION_COUNT
16-.    *    SECTIONS

Section:
0-N     str  LABEL (ends with '\0')
N+4     u32  TYPE
N+8     u32  DATA SIZE
N+12    u32  RESERVED (checksum)
N+16    *    DATA
*/

enum class SectionType : uint32_t {
  CODE    = 1,
  DATA    = 2,
  SYMBOLS = 3,
};

enum class SymbolFlags : uint16_t {
  LABEL     = 0x1,
  PROCEDURE = 0x2,
  VARIABLE  = 0x4,
};

struct Executable {
  struct Section {
    std::string label;
    SectionType type;
    uint32_t checksum;
    std::vector<uint8_t> data;

   public:
    size_t size() const;

    std::vector<uint8_t> toBytes() const;
    static Section fromBuffer(const uint8_t* data, size_t offset);

    std::vector<std::string> getStringLine() const;
    static const std::vector<std::string> getFieldNames();
  };

  uint32_t magic = XVM_BAD_MAGIC;
  uint32_t version = 0;
  uint32_t flags = 0;
  std::vector<Section> sections;

 public:
  bool hasSection(const std::string& label) const;
  Section& getSection(const std::string& label);
  const Section& getSection(const std::string& label) const;

  void disassemble() const;

  std::vector<uint8_t> toBytes() const;
  void toFile(const std::string& filename) const;

  static Executable fromBuffer(const uint8_t* data);
  static Executable fromFile(const std::string& filename);
};

struct SymbolTable {
  struct Symbol {
    uint32_t address;
    uint16_t flags;
    uint16_t size;
    std::string label;

    bool isLabel() const;
    bool isProcedure() const;
    bool isVariable() const;

    std::vector<std::string> getStringLine() const;
    static const std::vector<std::string> getFieldNames();
  };

  std::vector<Symbol> symbols;

  void addSymbol(uint32_t address, const std::string& label, uint16_t flags = (uint16_t)SymbolFlags::LABEL, uint16_t data_width = 0);

  Symbol& getByAddress(uint32_t address);
  Symbol& getByLabel(const std::string& label);

  bool hasAddress(uint32_t address) const;
  bool hasLabel(const std::string& label) const;

  Executable::Section toSection(const std::string& label = "symbols") const;

  static SymbolTable fromSection(const Executable::Section& section);
};

std::string sectionTypeToString(SectionType type);

} /* namespace xvm */

#endif