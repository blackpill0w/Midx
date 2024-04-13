# Midx
Program to index music files and their metadata for linux in C++20 (with Python bindings),
with the intention to be used as a backend for a music player.<br>
Given a directory (or directories), it searches for music files, extracts metadata and stores everything
in an sqlite database. Generate the Doxygen docs for details.
# Dependenies
Cmake will try to download & compile them, but I am not very good with it, so it might fail :)
- [cmake](https://cmake.org)
- [taglib](https://taglib.org)
# Using the library
- Download the library
- Add it from your project's `CmakeLists.txt`:
```cmake
add_subdirectory("deps/Midx")
```
- Include header files:
```cmake
get_target_property(MIDX_INCLUDE_DIRS Midx MIDX_INCLUDE_DIRS)
include_directories("${MIDX_INCLUDE_DIRS}")
```
# Build
Run this from the root directory of the project.
```bash
mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .
```
# TODO
- Testing.
- Make it cross platform.
