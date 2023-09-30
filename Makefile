# xvm

export TOPDIR    := $(shell pwd)
export BUILD_DIR := $(TOPDIR)/build
# export PREFIX    ?= $(BUILD_DIR)

export CXX       := g++
export CXXFLAGS  := -std=c++17 -I$(BUILD_DIR)/include -Wno-unsequenced


TARGET  := $(BUILD_DIR)/bin/xvm
SRC     :=  src/main.cc \
            src/vm.cc \
            src/abi.cc \
            src/bytecode.cc \
            src/executable.cc \
            src/syscalls.cc \
            src/config.cc \
            src/log.cc \
            src/loader.cc \
            src/linker.cc \
            src/assembler.cc \
            src/utils.cc \
            src/bus.cc \
            src/devices/ram.cc \
            src/syscalls/io.cc \
            src/syscalls/sleep.cc \
            src/syscalls/breakpoint.cc

$(info [x] xvm v0.1.2 dev)

ifeq ($(DEBUG),1)
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

build: install-headers install-lib
	for file in $(SRC); do \
		echo "[:] Compile $$file"; \
		$(CXX) -c $(CXXFLAGS) $$file -o "$(BUILD_DIR)/obj/$$(echo $${file%.*} | sed 's/\//_/g').o"; \
	done
	$(info [+] Build $(TARGET))
	$(CXX) $(CXXFLAGS) $(BUILD_DIR)/obj/*.o -o $(TARGET)

install-lib: prepare
	$(info [+] Install libraries)
	cp -r lib $(BUILD_DIR)/lib/xvm

install-headers: prepare
	$(info [+] Install headers)
	cp -r include $(BUILD_DIR)/include/xvm

prepare:
	$(info [+] Prepare folders)
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/bin
	mkdir -p $(BUILD_DIR)/include
	mkdir -p $(BUILD_DIR)/lib
	mkdir -p $(BUILD_DIR)/obj
	rm -rf $(BUILD_DIR)/include/xvm
	rm -rf $(BUILD_DIR)/lib/xvm

clean:
	$(info [+] Clean build)
	rm -rf $(BUILD_DIR)

$(V).SILENT:
