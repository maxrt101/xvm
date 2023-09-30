#include <xvm/linker.h>
#include <xvm/bytecode.h>
#include <xvm/version.h>
#include <xvm/config.h>
#include <xvm/utils.h>
#include <xvm/abi.h>
#include <xvm/log.h>

#include <cstdlib>

using namespace xvm;

static void patchAddressingMode(u8* code, i32 labelAddress, i32 mentionAddress, u8 argumentNumber) {
  using namespace abi;

  if (labelAddress - mentionAddress < 0) {
    // NRO
    if (argumentNumber == 1) {
      auto mode2 = extractModeArg2(code[mentionAddress - 2]);
      code[mentionAddress - 2] = encodeFlags(NRO, mode2);
    } else if (argumentNumber == 2) {
      auto mode1 = extractModeArg1(code[mentionAddress - 6]);
      code[mentionAddress - 6] = encodeFlags(mode1, NRO);
    }
  } else {
    // PRO
    if (argumentNumber == 1) {
      auto mode2 = extractModeArg2(code[mentionAddress - 2]);
      code[mentionAddress - 2] = encodeFlags(PRO, mode2);
    } else if (argumentNumber == 2) {
      auto mode1 = extractModeArg1(code[mentionAddress - 6]);
      code[mentionAddress - 6] = encodeFlags(mode1, PRO);
    }
  }
}

xvm::Executable xvm::link(const std::vector<Executable>& objects) {
  xvm::Executable result;

  u32 globalCodeOffset = 0;
  std::vector<u32> codeSectionOffsets;

  std::vector<u8> code;

  SymbolTable symbolTable, resultSymbolTable;
  RelocationTable relocationTable, resultRelocationTable;

  for (auto& object : objects) {
    codeSectionOffsets.push_back(globalCodeOffset);
    auto codeSection = object.getSection("code");
    for (auto byte : codeSection.data) {
      code.push_back(byte);
    }
    globalCodeOffset += codeSection.data.size();
  }
  for (int i = 0; i < objects.size(); i++) {
    if (!objects[i].hasSection("symbols")) {
      fatal("No 'symbols' section present in object file, can't link");
      die();
    }

    auto objectSymbolTable = SymbolTable::fromSection(objects[i].getSection("symbols"));

    for (const auto& symbol : objectSymbolTable.symbols) {
      symbolTable.addSymbol(symbol.address + codeSectionOffsets[i], symbol.label, symbol.flags, symbol.size);
    }
  }

  for (int i = 0; i < objects.size(); i++) {
    if (!objects[i].hasSection("relocations")) {
      fatal("No 'relocations' section present in object file, can't link");
      die();
    }

    auto objectRelocationTable = RelocationTable::fromSection(objects[i].getSection("relocations"));
  
    for (auto relocation : objectRelocationTable.relocations) {
      for (auto& mention : relocation.mentions) {
        mention.address += codeSectionOffsets[i];
      }
      relocationTable.relocations.push_back(relocation);
    }
  }

  for (const auto& symbol : symbolTable.symbols) {
    bool hasNonExternCounterpart = std::find_if(
      symbolTable.symbols.begin(),
      symbolTable.symbols.end(),
      [&symbol](const auto& symbol2) {
        return !symbol2.isExtern() && symbol2.label == symbol.label;
      }
    ) != symbolTable.symbols.end();

    if (symbol.isExtern() && hasNonExternCounterpart) {
      continue;
    }

    resultSymbolTable.symbols.push_back(symbol);
  }

  for (const auto& relocation : relocationTable.relocations) {
    if (resultSymbolTable.hasLabel(relocation.label)) {
      const auto& symbol = resultSymbolTable.getByLabel(relocation.label);

      for (const auto& mention : relocation.mentions) {
        abi::N32 value;
        if (config::asBool("pic")) {
          patchAddressingMode(code.data(), symbol.address, mention.address, mention.argumentNumber);
          value._i32 = std::abs(symbol.address - mention.address); // TODO: fix signed/unsiged mess
        } else {
          value._i32 = symbol.address;
        }
        code[mention.address]   = value._u8[0];
        code[mention.address+1] = value._u8[1];
        code[mention.address+2] = value._u8[2];
        code[mention.address+3] = value._u8[3];
      }
    } else {
      resultRelocationTable.relocations.push_back(relocation);
    }
  }

  result.magic = XVM_MAGIC;
  result.version = XVM_VERSION_CODE;
  result.flags = 0;

  result.sections.push_back(Executable::Section(
    SectionType::CODE,
    "code",
    code,
    0
  ));

  result.sections.push_back(resultSymbolTable.toSection());
  result.sections.push_back(resultRelocationTable.toSection());

  return result;
}
