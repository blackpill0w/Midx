cmake_minimum_required(VERSION 3.14)
project(Midx CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
string(CONCAT CMAKE_CXX_FLAGS
             "${CMAKE_CXX_FLAGS}"
             " -Wall -Wextra"
             " -Wfloat-conversion -Wsign-conversion"
             " -Wshadow -Wduplicated-cond"
             " -Wduplicated-branches -Wlogical-op"
             " -Wnull-dereference -Wuseless-cast"
             " -Wformat=2")

option(MIDX_BUILD_TESTS "Whether to build tests" FALSE)

# SqliteCpp
add_subdirectory(deps/SQLiteCpp)

# TagLib
add_subdirectory(deps/taglib-1.13)

# pybind11
include_directories(deps/pybind11/include)

# spdlog
include_directories(deps/spdlog/include)

add_library(Midx STATIC src/midx.cpp)

target_link_libraries(Midx
   SQLiteCpp
   tag
)

if (MIDX_BUILD_TESTS)
   add_executable(test src/main.cpp)
   target_link_libraries(test Midx)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Copy root/build/compile_commands.json to root/
if (EXISTS "${CMAKE_BINARY_DIR}/compile_commands.json")
   FILE(COPY ${CMAKE_BINARY_DIR}/compile_commands.json DESTINATION ${CMAKE_HOME_DIRECTORY})
endif()
