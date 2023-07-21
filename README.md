# Music-Indexer
Program to index music files and their metadata in C++20, with the intention to be used as a backend for a music player.
# Dependenies
Cmake will try to download & compile them, but I am not very good with it, so it might fail :)
- [cmake](https://cmake.org)
# Build
Run this from the root directory of the project.
```bash
mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_WITH_NIX=OFF .. && cmake --build .
```
### Note
If you have [nix](https://github.com/NixOS/nix) installed remove the `-DBUILD_WITH_NIX=OFF` option
and start a `nix-shell` before running cmake.
```
# TODO
- Indexing cover art.
