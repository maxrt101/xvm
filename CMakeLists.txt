cmake_minimum_required(VERSION 3.27)
project(xvm CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-I ${CMAKE_CURRENT_LIST_DIR}/include")

set(INCLUDES
    include/xvm/abi.h
    include/xvm/assembler.h
    include/xvm/bus.h
    include/xvm/bytecode.h
    include/xvm/config.h
    include/xvm/executable.h
    include/xvm/linker.h
    include/xvm/loader.h
    include/xvm/log.h
    include/xvm/stack.h
    include/xvm/syscalls.h
    include/xvm/utils.h
    include/xvm/version.h
    include/xvm/vm.h
    include/xvm/devices/ram.h
    include/xvm/devices/video.h
)

set(SOURCES
    src/main.cc
    src/vm.cc
    src/abi.cc
    src/bytecode.cc
    src/executable.cc
    src/syscalls.cc
    src/config.cc
    src/log.cc
    src/loader.cc
    src/linker.cc
    src/assembler.cc
    src/utils.cc
    src/bus.cc
    src/devices/ram.cc
    src/syscalls/io.cc
    src/syscalls/sleep.cc
    src/syscalls/breakpoint.cc
)

if(XVM_FEATURE_VIDEO)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DXVM_FEATURE_VIDEO=1")
    set(SOURCES "${INCLUDES} include/xvm/devices/video.h")
    set(SOURCES "${SOURCES} src/devices/video.c")
endif ()

add_executable(xvm ${INCLUDES} ${SOURCES})
