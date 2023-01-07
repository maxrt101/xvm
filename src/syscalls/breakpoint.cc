#include <xvm/syscalls.h>
#include <xvm/devices/ram.h>
#include <xvm/bytecode.h>
#include <xvm/config.h>
#include <xvm/utils.h>
#include <xvm/log.h>
#include <xvm/abi.h>

#include <iostream>
#include <string>

#define CHECK_ADDR_OUTPUT(x, s) \
  do { \
    if ((x) == -1) { \
      error("Invalid value ('%s')", (s).c_str()); \
      continue; \
    } \
  } while (0)

void xvm::sys_breakpoint(VM* vm) {
  bool running = true;
  while (running) {
    std::string line;
    printf("brkp# ");
    std::getline(std::cin, line);
    if (line.empty()) continue;
    std::vector<std::string> tokens = splitString(line, ' ');
    if (tokens[0] == "help") {
      if (tokens.size() == 2) {
        if (tokens[1] == "help") {
          printf("Usage: help [COMMAND]\n");
        } else if (tokens[1] == "halt") {
          printf("Usage: halt\n");
        } else if (tokens[1] == "continue") {
          printf("Usage: continue\n");
        } else if (tokens[1] == "reset") {
          printf("Usage: reset\n");
        } else if (tokens[1] == "push") {
          printf("Usage: push VALUE");
        } else if (tokens[1] == "pop") {
          printf("Usage: pop\n");
        } else if (tokens[1] == "set") {
          printf("Usage: set [TYPE=i8] ADDR VALUE\n");
        } else if (tokens[1] == "jump") {
          printf("Usage: jump ADDR\n");
        } else if (tokens[1] == "call") {
          printf("Usage: call ADDR\n");
        } else if (tokens[1] == "getopt") {
          printf("Usage: getopt NAME\n");
        } else if (tokens[1] == "setopt") {
          printf("Usage: setopt NAME VALUE\n");
        } else if (tokens[1] == "config") {
          printf("Usage: config\n");
        } else if (tokens[1] == "print") {
          printf(
            "Usage: print stack\n"
            "Usage: print ram ADDR LEN\n"
            "Usage: print code ADDR LEN\n"
            "Usage: print TYPE ADDR\n"
          );
        }
      } else {
        printf("Available commands: help halt continue reset print getopt setopt config push pop set jump call\n");
      }
    } else if (tokens[0] == "halt" || tokens[0] == "exit" || tokens[0] == "quit" || tokens[0] == "q") {
      vm->stop();
      running = false;
    } else if (tokens[0] == "continue" || tokens[0] == "c") {
      running = false;
    } else if (tokens[0] == "push") {
      if (tokens.size() != 2) {
        error("Usage: push VALUE");
        continue;
      }
      int val = utils::getAddr(vm, tokens[1]);
      CHECK_ADDR_OUTPUT(val, tokens[1]);
      vm->getStack().push(val);
    } else if (tokens[0] == "pop") {
      printf("%d\n", vm->getStack().pop());
    } else if (tokens[0] == "jump" || tokens[0] == "j") {
      if (tokens.size() != 2) {
        error("Usage: jump ADDR");
        continue;
      }
      int val = utils::getAddr(vm, tokens[1]);
      CHECK_ADDR_OUTPUT(val, tokens[1]);
      vm->jump(val);
    } else if (tokens[0] == "call") {
      if (tokens.size() != 2) {
        error("Usage: call ADDR");
        continue;
      }
      int val = utils::getAddr(vm, tokens[1]);
      CHECK_ADDR_OUTPUT(val, tokens[1]);
      vm->call(val);
    } else if (tokens[0] == "reset" || tokens[0] == "r") {
      vm->reset();
    } else if (tokens[0] == "set" || tokens[0] == "s") {
      std::string type = "i8";
      int32_t addr = 0;
      int32_t value = 0;
      if (tokens.size() == 3) {
        addr = utils::getAddr(vm, tokens[1]);
        CHECK_ADDR_OUTPUT(addr, tokens[1]);
        value = utils::getAddr(vm, tokens[2]);
        CHECK_ADDR_OUTPUT(value, tokens[2]);
      } else if (tokens.size() == 4) {
        type = tokens[1];
        addr = utils::getAddr(vm, tokens[2]);
        CHECK_ADDR_OUTPUT(addr, tokens[2]);
        value = utils::getAddr(vm, tokens[3]);
        CHECK_ADDR_OUTPUT(value, tokens[3]);
      } else {
        error("Usage: set [TYPE=i8] ADDR VALUE");
      }
      if (type == "i8") {
        vm->getBus().write(addr, value);
      } else if (type == "i16") {
        abi::N32 n;
        n.i16[0] = value;
        vm->getBus().write(addr, n.u8[0]);
        vm->getBus().write(addr+1, n.u8[1]);
      } else if (type == "i32") {
        abi::N32 n;
        n.i32 = value;
        vm->getBus().write(addr, n.u8[0]);
        vm->getBus().write(addr+1, n.u8[1]);
        vm->getBus().write(addr+2, n.u8[2]);
        vm->getBus().write(addr+3, n.u8[3]);
      } else {
        error("Unknown type");
      }
    } else if (tokens[0] == "getopt") {
      if (tokens.size() != 2) {
        error("Usage: getopt NAME");
        continue;
      }
      printf("%s\n", config::getAsString(tokens[1]).c_str());
    } else if (tokens[0] == "setopt") {
      if (tokens.size() != 3) {
        error("Usage: getopt NAME VALUE");
        continue;
      }
      config::setFromString(tokens[1], tokens[2]);
    } else if (tokens[0] == "config" || tokens[0] == "conf" || tokens[0] == "cfg") {
      auto keys = config::getKeys();
      for (auto& key : keys) {
        printf("%s: '%s'\n", key.c_str(), config::getAsString(key).c_str());
      }
    } else if (tokens[0] == "print" || tokens[0] == "p") {
      if (tokens.size() > 1) {
        if (tokens[1] == "stack" || tokens[1] == "s") {
          printf("[ ");
          for (int i = vm->getStack().size()-1; i >= 0; i--) {
            printf("%d ", vm->getStack().peek(i));
          }
          printf("]\n");
        } else if (tokens[1] == "ram" || tokens[1] == "r") {
          if (tokens.size() != 4) {
            error("Usage: 'print ram ADDR LEN'");
            continue;
          }
          int addr = utils::getAddr(vm, tokens[2]);
          CHECK_ADDR_OUTPUT(addr, tokens[2]);
          int len = utils::getAddr(vm, tokens[3]);
          CHECK_ADDR_OUTPUT(len, tokens[3]);
          auto ram = (bus::device::RAM*)vm->getBus().getDeviceByAddress(addr);
          abi::hexdump(ram->getBuffer() + addr, len);
        } else if (tokens[1] == "code" || tokens[1] == "c") {
          if (tokens.size() != 4) {
            error("Usage: 'print code ADDR LEN'");
            continue;
          }
          int addr = utils::getAddr(vm, tokens[2]);
          CHECK_ADDR_OUTPUT(addr, tokens[2]);
          int len = utils::getAddr(vm, tokens[3]);
          CHECK_ADDR_OUTPUT(len, tokens[3]);
          auto ram = (bus::device::RAM*)vm->getBus().getDeviceByAddress(addr);
          for (int i = 0; i < len; ) {
            i = xvm::abi::disassembleInstruction(ram->getBuffer() + addr, i);
          }
        } else {
          if (tokens.size() != 3) {
            error("Usage: 'print TYPE ADDR'");
            continue;
          }
          int addr = utils::getAddr(vm, tokens[2]);
          CHECK_ADDR_OUTPUT(addr, tokens[2]);
          if (tokens[1] == "i8") {
            printf("0x%x\n", vm->getBus().read(addr));
          } else if (tokens[1] == "i16") {
            abi::N32 n;
            n.u8[0] = vm->getBus().read(addr);
            n.u8[1] = vm->getBus().read(addr+1);
            printf("0x%x\n", n.i16[0]);
          } else if (tokens[1] == "i32") {
            abi::N32 n;
            n.u8[0] = vm->getBus().read(addr);
            n.u8[1] = vm->getBus().read(addr+1);
            n.u8[2] = vm->getBus().read(addr+2);
            n.u8[3] = vm->getBus().read(addr+3);
            printf("0x%x\n", n.i32);
          } else if (tokens[1] == "str") {
            printf("%s\n", utils::busReadString(vm, addr).c_str());
          } else {
            error("Unknown type");
          }
        }
      } else {
        error("Type 'help print' for usage");
      }
    } else {
      error("Unknown command");
    }
  }
}
