# Midx
Program to index music files and their metadata for linux in C++20 (with Python bindings),
with the intention to be used as a backend for a music player.
# Dependenies
Cmake will try to download & compile them, but I am not very good with it, so it might fail :)
- [cmake](https://cmake.org)
# Build
Run this from the root directory of the project.
```bash
mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .
```
# TODO
- Testing.
- Make it cross platform.
