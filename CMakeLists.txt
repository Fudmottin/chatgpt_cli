cmake_minimum_required(VERSION 3.20)
project(chatgpt_cli)
add_subdirectory(external/cppcodec)

# Policy for Boost (no-op on older CMake versions)
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake") # Add this line

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
option(BUILD_DEBUG "Build with debug info" OFF)
if(BUILD_DEBUG)
    set(CMAKE_BUILD_TYPE Debug)
else()
    set(CMAKE_BUILD_TYPE Release)
endif()

# Boost.JSON
find_package(Boost REQUIRED COMPONENTS json)

set(CPR_INCLUDE_DIRS "/usr/local/include")
set(CPR_LIBRARIES "/usr/local/lib/libcpr.a")
set(CPPCODEC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/cppcodec)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(Curses REQUIRED)
find_package(Editline REQUIRED)

add_executable(chatgpt_cli
    src/main.cpp
    src/openai_client.cpp
    src/chat_client.cpp
    src/openai_image.cpp
    src/utils.cpp
)

target_include_directories(chatgpt_cli PRIVATE ${CPR_INCLUDE_DIRS} ${EDITLINE_INCLUDE_DIRS} ${CURSES_INCLUDE_DIRS} ${CPPCODEC_INCLUDE_DIR})
target_link_libraries(chatgpt_cli PRIVATE nlohmann_json::nlohmann_json ${CPR_LIBRARIES} -lcurl ${EDITLINE_LIBRARIES} ${CURSES_LIBRARIES} cppcodec)

