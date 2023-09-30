#ifndef _XVM_EXECUTABLE_H_
#define _XVM_EXECUTABLE_H_ 1

#include <cstdint>
#include <string>
#include <vector>
#include <xvm/abi.h>

#define XVM_MAGIC 0xdeadbeef
#define XVM_SECTION_MAGIC 0xdeadbeef
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

enum class SectionType : u32 {
  CODE        = 1,
  DATA        = 2, // ?
  SYMBOLS     = 3,
  RELOCATIONS = 4,
  RUNINFO     = 5,
  // TODO: runinfo or some other section has to have a list of
  // dynamic mlibraries to be linked before execution (stdlib, etc)
};

enum class SymbolFlags : u32 {
  LABEL     = 0x1,
  PROCEDURE = 0x2,
  VARIABLE  = 0x4,
  ENTRY     = 0x8,
  EXTERN    = 0x10,
};

struct Executable {
  struct Section {
    u32 magic;
    SectionType type;
    u32 addr;
    u32 checksum;
    std::string label;
    std::vector<u8> data;

   public:
    Section() = default;
    Section(SectionType type, std::string label, std::vector<u8> data, u32 addr = -1U);

    size_t size() const;

    std::vector<u8> toBytes() const;
    static Section fromBuffer(const u8* data, size_t offset);

    std::vector<std::string> getStringLine() const;
    static const std::vector<std::string> getFieldNames();
  };

  u32 magic = XVM_BAD_MAGIC;
  u32 version = 0;
  u32 flags = 0;
  std::vector<Section> sections;

 public:
  bool hasSection(const std::string& label) const;
  Section& getSection(const std::string& label);
  const Section& getSection(const std::string& label) const;

  void disassemble() const;

  std::vector<u8> toBytes() const;
  void toFile(const std::string& filename) const;

  static Executable fromBuffer(const u8* data);
  static Executable fromFile(const std::string& filename);
};

// Special Sections

struct SymbolTable {
  struct Symbol {
    i32 address;
    u16 flags;
    u16 size;
    std::string label;

    bool isLabel() const;
    bool isProcedure() const;
    bool isVariable() const;
    bool isExtern() const;

    std::vector<std::string> getStringLine() const;
    static const std::vector<std::string> getFieldNames();
  };

  std::vector<Symbol> symbols;

  void addSymbol(i32 address, const std::string& label, u16 flags = (u16)SymbolFlags::LABEL, u16 data_width = 0);

  Symbol& getByAddress(i32 address);
  Symbol& getByLabel(const std::string& label);

  bool hasAddress(i32 address) const;
  bool hasLabel(const std::string& label) const;

  Executable::Section toSection(const std::string& label = "symbols") const;

  static SymbolTable fromSection(const Executable::Section& section);
};

struct RelocationTable {
  struct RelocationEntry {
    struct SymbolMention {
      i32 address;
      u8 argumentNumber;
    };

    std::string label;
    // RelocationType type; // FIXME: needed?
    std::vector<SymbolMention> mentions; // within executable (or even code section?)
  };

  std::vector<RelocationEntry> relocations;

  Executable::Section toSection(const std::string& label = "relocations") const;

  static RelocationTable fromSection(const Executable::Section& section);
};

std::string sectionTypeToString(SectionType type);

} /* namespace xvm */

#endif /* _XVM_EXECUTABLE_H_ */