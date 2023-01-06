# xvm

export TOPDIR   := $(shell pwd)
export PREFIX   := $(TOPDIR)/build

export CXX      := g++
export CXXFLAGS := -std=c++17 -I$(PREFIX)/include -Wno-unsequenced

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
            src/syscalls/sleep.cc \
            src/syscalls/breakpoint.cc

ifeq ("$(DEBUG)","1")
$(info [!] Debug on)
CXXFLAGS += -g3 -D_DEBUG
endif

ifeq ($(VIDEO),1)
$(info [+] Video support on)
CXXFLAGS += -lsdl2 -DXVM_FEATURE_VIDEO=1
SRC += src/devices/video.cc src/syscalls/video.cc
else
$(info [-] Video support off)
endif


.PHONY: build

build: install_headers install_lib
	$(info [+] Building $(TARGET))
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

install_lib: prepare
	$(info [+] Installing libraries)
	cp -r lib $(PREFIX)/lib/xvm

install_headers: prepare
	$(info [+] Installing headers)
	cp -r include $(PREFIX)/include/xvm

prepare:
	$(info [+] Preparing folders)
	mkdir -p $(PREFIX)
	mkdir -p $(PREFIX)/bin
	mkdir -p $(PREFIX)/include
	mkdir -p $(PREFIX)/lib
	rm -rf $(PREFIX)/include/xvm
	rm -rf $(PREFIX)/lib/xvm

clean::
	$(info [+] Cleaning)
	rm -rf $(TARGET)
	rm -rf $(PREFIX)/include/xvm

$(V).SILENT:
