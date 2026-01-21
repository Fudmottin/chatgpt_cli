cmake_minimum_required(VERSION 3.20)
project(clanker LANGUAGES CXX)

# Avoid inheriting user environment linker flags into the cache.
# Users can still pass flags explicitly via CMake toolchain/presets if desired.
set(CMAKE_EXE_LINKER_FLAGS "" CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS "" CACHE STRING "" FORCE)
set(CMAKE_MODULE_LINKER_FLAGS "" CACHE STRING "" FORCE)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CLANKER_WERROR "Treat warnings as errors" OFF)

add_executable(clanker
    src/main.cpp

    src/clanker/shell.cpp
    src/clanker/line_editor.cpp
    src/clanker/history.cpp
    src/clanker/lexer.cpp
    src/clanker/parser.cpp
    src/clanker/executor.cpp
    src/clanker/builtins.cpp
    src/clanker/builtin_core.cpp
    src/clanker/builtin_llm.cpp
    src/clanker/process.cpp
    src/clanker/signals.cpp
    src/clanker/util.cpp

    src/clanker_llm/registry.cpp
    src/clanker_llm/backend_stub.cpp
    src/clanker_llm/redact.cpp
)

target_include_directories(clanker PRIVATE
    src
)

target_compile_options(clanker PRIVATE
    -Wall -Wextra -Wpedantic
)

if(CLANKER_WERROR)
    target_compile_options(clanker PRIVATE -Werror)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

add_executable(clanker_tests
    src/tests/test_main.cpp
)

target_compile_options(clanker_tests PRIVATE
    -Wall -Wextra -Wpedantic
)

add_test(
    NAME clanker_smoke
    COMMAND clanker_tests $<TARGET_FILE:clanker>
)

