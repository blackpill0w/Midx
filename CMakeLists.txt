cmake_minimum_required(VERSION 3.14)
project(MusicIndexer CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# SqliteCpp
find_package(Sqlite3)
if (NOT SQLiteCpp_FOUND)
   #   add_subdirectory(dependencies/SQLiteCpp)
endif()
find_package(SQLiteCpp)
if (NOT SQLiteCpp_FOUND)
   # add_subdirectory(dependencies/SQLiteCpp)
endif()
# Taglib
find_package(taglib)
if (NOT taglib_FOUND)
   # add_subdirectory(dependencies/taglib-1.13)
endif()

add_library(MusicIndexer STATIC src/dbops.cpp)
target_link_libraries(MusicIndexer tag SQLiteCpp)

add_executable(test src/main.cpp)
target_link_libraries(test MusicIndexer)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
