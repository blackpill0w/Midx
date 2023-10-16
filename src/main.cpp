#include <iostream>
#include <filesystem>
#include <SQLiteCpp/SQLiteCpp.h>

#include "./music_indexer.hpp"

using namespace MusicIndexer;
namespace fs = std::filesystem;

int main() {
  SQLite::Database db{"db.sqlite", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE};

  MusicIndexer::init_database(db);

  insert<MusicDir>(db, "/home/blackpill0w/Music/Aphex Twin/Drukqs [2001]");

  MusicIndexer::build_music_library(db);

  for (auto &t : MusicIndexer::get_all<Track>(db)) {
    std::cout << t.file_path << "\n";
  }
}
