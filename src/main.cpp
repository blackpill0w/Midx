#include <iostream>
#include <fstream>

#include "./music_indexer.hpp"

namespace MI = MusicIndexer;

int main() {
  SQLite::Database db{"../music_library.sqlite",
                      SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE};
  MI::init_database(db);

  //MI::insert<MI::MusicDir>(db, "/home/blackpill0w/Music");
  //MI::build_music_library(db);

  std::cout << "\t\t--------------------\n";

  MI::remove_music_dir(db, "/home/blackpill0w/Music");
}
