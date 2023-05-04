cmake_minimum_required(VERSION 3.12)
project(chatgpt_cli)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake") # Add this line

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json REQUIRED)
set(CPR_INCLUDE_DIRS "/usr/local/include")
set(CPR_LIBRARIES "/usr/local/lib/libcpr.a")
find_package(Curses REQUIRED)
find_package(Editline REQUIRED)

add_executable(chatgpt_cli main.cpp openai_client.cpp chat_client.cpp utils.cpp)

target_include_directories(chatgpt_cli PRIVATE ${CPR_INCLUDE_DIRS} ${EDITLINE_INCLUDE_DIRS} ${CURSES_INCLUDE_DIRS})
target_link_libraries(chatgpt_cli PRIVATE nlohmann_json::nlohmann_json ${CPR_LIBRARIES} -lcurl ${EDITLINE_LIBRARIES} ${CURSES_LIBRARIES})

