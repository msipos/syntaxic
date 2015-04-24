project(SyntaxicUtilities)
cmake_minimum_required(VERSION 2.8.11)

# Written in C99 rather than C++ to ease portability
set(CMAKE_C_FLAGS "-std=c99")

include_directories(src)

add_executable(syntaxic_remote_editor src/remote_edit.c src/pty_wrapper.c)

add_executable(syntaxic_local_wrapper src/local_edit.c src/pty_wrapper.c)