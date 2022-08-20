#include <xvm/vm.h>
#include <xvm/abi.h>
#include <xvm/log.h>
#include <xvm/utils.h>
#include <xvm/config.h>
#include <xvm/binary.h>
#include <xvm/version.h>
#include <xvm/assembler.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>


static void printVersion() {
  printf("xvm v%s\n", XVM_VERSION);
}

static void printHelp(const char* argv0) {
  printVersion();
  printf("Usage: %s COMMAND [OPTIONS]\n", argv0);
  printf("Commands:\n");
  printf("  help          - Shows this message\n");
  printf("  version       - Shows version\n");
  printf("  compile FILE  - Compiles FILE, output binary can be specified with -o\n");
  printf("  run FILE      - Runs compiled file\n");
  printf("  runsrc FILE   - Runs source file direclty (without saving binary)\n");
  printf("  disasm FILE   - Disassembles compiled file\n");
  printf("Options:\n");
  printf("  -s, --setopt OPTION=VALUE - Sets option\n");
  printf("  -o, --output FILE         - Sets output file\n");
  printf("  -i, --include FOLDER      - Adds include folder\n");
  printf("  -r, --raw                 - Outputs raw compiled code\n");
  printf("  -d, --debug               - Turns on debug flag\n");
  printf("  -v, --verbose             - Turns on verbose flag\n");
}

static int compile(const std::string& filename, const std::string& output, std::vector<std::string>& includes) {
  if (!xvm::isFileExists(filename)) {
    xvm::error("File not exists: '%s'", filename.c_str());
    return -1;
  }

  std::ifstream sourceFile(filename);
  std::string source((std::istreambuf_iterator<char>(sourceFile)), std::istreambuf_iterator<char>());

  xvm::Assembler assembler(filename, source);
  assembler.setIncludeFolders(includes);

  xvm::Executable exe;
  int ret = assembler.assemble(exe);
  if (ret) {
    xvm::error("Error assembling '%s'", filename.c_str());
    return -1;
  }

  if (xvm::config::getBool("hexdump")) {
    xvm::abi::hexdump(exe.data.data(), exe.data.size());
  }

  if (xvm::config::getBool("disasm")) {
    printf("=== disassembly ===\n");
    for (int i = 0; i < exe.data.size(); ) {
      i = xvm::abi::disassembleInstruction(exe.data.data(), i);
    }
  }

  exe.toFile(output);
  return 0;
}

static int run(const std::string& filename) {
  if (!xvm::isFileExists(filename)) {
    xvm::error("File not exists: '%s'", filename.c_str());
    return -1;
  }

  xvm::Executable exe = xvm::Executable::fromFile(filename);

  if (exe.magic == XVM_BAD_MAGIC) {
    xvm::error("Error opening/reading executable");
  }

  if (xvm::config::getBool("hexdump")) {
    xvm::abi::hexdump(exe.data.data(), exe.data.size());
  }

  if (xvm::config::getBool("disasm")) {
    printf("=== disassembly ===\n");
    for (int i = 0; i < exe.data.size(); ) {
      i = xvm::abi::disassembleInstruction(exe.data.data(), i);
    }
  }

  xvm::VM vm(xvm::config::getInt("ram_size"));
  vm.loadRegion(0, exe.data.data(), exe.data.size());
  if (xvm::config::getBool("disasm")) {
    printf("=== execution ===\n");
  }
  vm.run();
  return 0;
}

static int runSrc(const std::string& filename, std::vector<std::string>& includes) {
  if (!xvm::isFileExists(filename)) {
    xvm::error("File not exists: '%s'", filename.c_str());
    return -1;
  }

  std::ifstream sourceFile(filename);
  std::string source((std::istreambuf_iterator<char>(sourceFile)), std::istreambuf_iterator<char>());

  xvm::Assembler assembler(filename, source);
  assembler.setIncludeFolders(includes);

  xvm::Executable exe;
  int ret = assembler.assemble(exe);
  if (ret) {
    xvm::error("Error assembling '%s'", filename.c_str());
    return -1;
  }

  if (xvm::config::getBool("hexdump")) {
    xvm::abi::hexdump(exe.data.data(), exe.data.size());
  }

  if (xvm::config::getBool("disasm")) {
    printf("=== disassembly ===\n");
    for (int i = 0; i < exe.data.size(); ) {
      i = xvm::abi::disassembleInstruction(exe.data.data(), i);
    }
  }

  xvm::VM vm(xvm::config::getInt("ram_size"));
  vm.loadRegion(0, exe.data.data(), exe.data.size());
  if (xvm::config::getBool("disasm")) {
    printf("=== execution ===\n");
  }
  vm.run();
  return 0;
}

static int disasm(const std::string& filename) {
  if (!xvm::isFileExists(filename)) {
    xvm::error("File not exists: '%s'", filename.c_str());
    return -1;
  }

  xvm::Executable exe = xvm::Executable::fromFile(filename);
  
  if (exe.magic == XVM_BAD_MAGIC) {
    xvm::error("Error opening/reading executable");
  }

  printVersion();
  printf("File: %s\nMagic: 0x%x\nVersion: 0x%x\n", filename.c_str(), exe.magic, exe.version);

  if (xvm::config::getBool("hexdump")) {
    xvm::abi::hexdump(exe.data.data(), exe.data.size());
  }

  printf("=== disassembly ===\n");
  for (int i = 0; i < exe.data.size(); ) {
    i = xvm::abi::disassembleInstruction(exe.data.data(), i);
  }

  return 0;
}

int main(int argc, char ** argv) {
  xvm::config::initialize();

  std::string command; // repl?
  std::string inputFilename;
  std::string outputFilename = "out.xbin";

  std::vector<std::string> includeFolders;

  for (int i = 1; i < argc; i++) {
    if (!strcmp("-s", argv[i]) || !strcmp("--setopt", argv[i])) {
      if (i+1 >= argc) {
        xvm::error("Expected 'key=value' after '%s'", argv[i]);
        return -1;
      }
      std::string setopt = argv[++i];
      int idx = setopt.find("=");
      if (idx == std::string::npos) {
        xvm::error("Expected 'key=value'");
        return -1;
      }
      std::string key = setopt.substr(0, idx), value = setopt.substr(idx+1);
      xvm::config::setFromString(key, value);
    } else if (!strcmp("-o", argv[i]) || !strcmp("--output", argv[i])) {
      outputFilename = argv[++i];
    } else if (!strcmp("-i", argv[i]) || !strcmp("--include", argv[i])) {
      if (i+1 >= argc) {
        xvm::error("Expected folder after '%s'", argv[i]);
        return -1;
      }
      includeFolders.push_back(argv[++i]);
    } else if (!strcmp("-r", argv[i]) || !strcmp("--raw", argv[i])) {
      xvm::config::setBool("raw", true);
      outputFilename = "out.bin";
    } else if (!strcmp("-d", argv[i]) || !strcmp("--debug", argv[i])) {
      xvm::config::setBool("debug", true);
    } else if (!strcmp("-v", argv[i]) || !strcmp("--verbose", argv[i])) {
      xvm::config::setBool("verbose", true);
    } else {
      if (command.empty()) {
        command = argv[i];
      } else if (inputFilename.empty()) {
        inputFilename = argv[i];
      } else {
        xvm::error("Unrecognized parameter: '%s'", argv[i]);
        return -1;
      }
    }
  }

  if (command == "") {
    xvm::error("No command supplied, type '%s help' to get help", argv[0]);
    return -1;
  } else if (command == "help" || command == "-h" || command == "--help") {
    printHelp(argv[0]);
  } else if (command == "version") {
    printVersion();
  } else if (command == "compile") {
    return compile(inputFilename, outputFilename, includeFolders);
  } else if (command == "run") {
    run(inputFilename);
  } else if (command == "runsrc") {
    return runSrc(inputFilename, includeFolders);
  } else if (command == "disasm") {
    return disasm(inputFilename);
  } else {
    xvm::error("Unknown command: '%s'", command.c_str());
  }

  return 0;
}
