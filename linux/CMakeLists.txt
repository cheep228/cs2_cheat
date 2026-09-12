cmake_minimum_required(VERSION 3.16)
project(cs2_linux_skinchanger LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT UNIX OR APPLE)
    message(FATAL_ERROR "linux/ targets the native Linux CS2 client only.")
endif()

add_library(cs2_linux_skinchanger SHARED
    src/log.cpp
    src/mem.cpp
    src/modules.cpp
    src/sigscan.cpp
    src/schema_system.cpp
    src/fields.cpp
    src/entity.cpp
    src/config.cpp
    src/skinchanger.cpp
    src/main.cpp)

target_include_directories(cs2_linux_skinchanger PRIVATE src)
target_compile_definitions(cs2_linux_skinchanger PRIVATE _GNU_SOURCE)
target_compile_options(cs2_linux_skinchanger PRIVATE -Wall -Wextra)
target_link_libraries(cs2_linux_skinchanger PRIVATE dl pthread)
set_target_properties(cs2_linux_skinchanger PROPERTIES OUTPUT_NAME "cs2_skinchanger")
