cmake_minimum_required(VERSION 3.23)
project(mon_c_interpreter C)

set(CMAKE_C_STANDARD 99)

add_executable(mon_c_interpreter src/main.c src/common.h src/chunk.h src/chunk.c src/memory.h src/memory.c src/debug.h src/debug.c src/value.h src/value.c src/vm.h src/vm.c src/compiler.h src/compiler.c src/scanner.h)
