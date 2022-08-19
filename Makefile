# xvm

export TOPDIR   := $(shell pwd)
export PREFIX   := $(TOPDIR)/build

export CXX      := clang++
export CXXFLAGS := -std=c++17 -I$(TOPDIR)/include -I$(PREFIX)/include -Wno-unsequenced

TARGET  := $(PREFIX)/bin/xvm
SRC     :=  src/main.cc \
            src/vm.cc \
            src/abi.cc \
            src/bytecode.cc \
            src/binary.cc \
            src/syscalls.cc \
            src/config.cc \
            src/log.cc \
            src/assembler.cc \
            src/utils.cc \
            src/bus.cc \
            src/devices/ram.cc \
            src/syscalls/io.cc \
            src/syscalls/breakpoint.cc

ifeq ("$(DEBUG)","1")
$(info [!] Debug on)
CXXFLAGS += -g3 -D_DEBUG
endif

.PHONY: build

build: prepare
	$(info [+] Building $(TARGET))
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

prepare:
	$(info [+] Preparing folders)
	mkdir -p $(PREFIX)
	mkdir -p $(PREFIX)/bin

clean::
	$(info [+] Cleaning)
	rm -rf $(TARGET)

$(V).SILENT:
