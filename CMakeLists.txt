cmake_minimum_required(VERSION 3.14)
project(Music-Indexer CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# Sqlite3
find_package(SQLite3)
#include_directories(${SQLite3_INCLUDE_DIRS})
# sqlite_modern_cpp
include_directories(dependencies/sqlite_modern_cpp/hdr)
# Taglib
#add_subdirectory(dependencies/taglib-1.13)

add_executable(music-indexer
   src/main.cpp
   src/database_operations.cpp)
target_link_libraries(music-indexer ${SQLite3_LIBRARIES} tag)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Copy root/build/compile_commands.json to root/
if (EXISTS "${CMAKE_BINARY_DIR}/compile_commands.json")
   FILE(COPY ${CMAKE_BINARY_DIR}/compile_commands.json DESTINATION ${CMAKE_HOME_DIRECTORY})
endif()