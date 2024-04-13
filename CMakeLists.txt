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
  " -Wformat=2 -fPIC")

option(MIDX_BUILD_TESTS "Whether to build tests" FALSE)
option(MIDX_PYTHON_BINDINGS "Whether to generate python bindings" FALSE)

set(BUILD_TESTING FALSE)
set(SQLITECPP_RUN_CPPLINT FALSE)

# SqliteCpp
add_subdirectory("deps/SQLiteCpp")

# TagLib
#set(BUILD_SHARED_LIBS OFF)
#add_subdirectory("deps/taglib-1.13/")
find_package(taglib REQUIRED)

set(MIDX_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${CMAKE_CURRENT_SOURCE_DIR}/deps/spdlog/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/deps/SQLiteCpp/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/deps/pybind11/include"
)
include_directories(${MIDX_INCLUDE_DIRS})

add_library(Midx STATIC src/midx.cpp)
set_target_properties(Midx PROPERTIES
  MIDX_INCLUDE_DIRS "${MIDX_INCLUDE_DIRS}")
target_link_libraries(Midx
   SQLiteCpp
   tag
)

# Generage python bindings
if (MIDX_PYTHON_BINDINGS)
  add_subdirectory(deps/pybind11)
  pybind11_add_module(midx src/midx.cpp src/midx_python_bindings.cpp)
  target_link_libraries(midx PUBLIC
    SQLiteCpp
    tag
  )
  string(CONCAT CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS}" " -flto=auto")
endif()

if (MIDX_BUILD_TESTS)
   add_executable(test src/main.cpp)
   target_link_libraries(test Midx)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Copy root/build/compile_commands.json to root/
if (EXISTS "${CMAKE_BINARY_DIR}/compile_commands.json")
   FILE(COPY ${CMAKE_BINARY_DIR}/compile_commands.json DESTINATION ${CMAKE_HOME_DIRECTORY})
endif()
