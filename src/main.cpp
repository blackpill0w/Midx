#include <iostream>
#include <fstream>

#include "./music_indexer.hpp"

namespace MI = MusicIndexer;

int main(int argc, const char *argv[]) {
  SQLite::Database db{"../music_library.sqlite",
                      SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE};
  MI::init_database(db);

  MI::insert<MI::MusicDir>(db, "/home/blackpill0w/Music/");

  MI::remove_music_dir(db, "/home/blackpill0w/Music/");
}
