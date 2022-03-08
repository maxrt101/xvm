
export TOPDIR   := $(shell pwd)
export PREFIX   := $(TOPDIR)/build

export CXX      := clang++
export CXXFLAGS := -std=c++17 -I$(TOPDIR)/include -I$(PREFIX)/include

TARGET 	:= $(PREFIX)/bin/xvm
SRC 		:=  src/main.cc \
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
				src/devices/ram.cc

ifeq ("$(DEBUG)","1")
CXXFLAGS += -g3 -D_DEBUG
endif

.PHONY: build

build: prepare
	$(info Building $(TARGET))
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

prepare:
	mkdir -p $(PREFIX)
	mkdir -p $(PREFIX)/bin

clean::
	rm -rf $(PREFIX)

$(V).SILENT:
