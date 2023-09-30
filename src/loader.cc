#include <xvm/loader.h>
#include <xvm/config.h>
#include <xvm/utils.h>
#include <xvm/log.h>

int xvm::load(VM& vm, Executable& exe) {
  if (exe.magic != XVM_MAGIC) {
    xvm::error(exe.magic == XVM_BAD_MAGIC ? "Error opening/reading executable" : "Bad file");
    return 1;
  }

  if (!exe.hasSection("code")) {
    xvm::error("Executable loading failed: Missing code section");
    return 1;
  }

  auto& code = exe.getSection("code");

  if (xvm::config::asBool("hexdump")) {
    printf("=== Hexdump ===\n");
    auto bytes = exe.toBytes();
    xvm::abi::hexdump(bytes.data(), bytes.size());
  }

  if (xvm::config::asBool("print-symbol-table") && exe.hasSection("symbols")) {
    printf("=== Symbols ===\n");
    auto table = xvm::SymbolTable::fromSection(exe.getSection("symbols"));
    xvm::printTable(xvm::SymbolTable::Symbol::getFieldNames(), table.symbols);
  }

  if (xvm::config::asBool("disasm")) {
    printf("=== Disassembly ===\n");
    if (xvm::config::asBool("fancy-disasm")) {
      exe.disassemble();
    } else {
      for (int i = 0; i < code.data.size(); ) {
        i = xvm::abi::disassembleInstruction(code.data.data(), i);
      }
    }
  }

  // xvm::VM vm(xvm::config::asInt("ram-size"));
  vm.loadRegion(0, code.data.data(), code.data.size());
  
  if (exe.hasSection("symbols")) {
    vm.loadSymbols(xvm::SymbolTable::fromSection(exe.getSection("symbols")));
  }

  return 0;
}
