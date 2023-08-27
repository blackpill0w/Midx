#include <iostream>
#include <SQLiteCpp/SQLiteCpp.h>

#include "./music_indexer.hpp"

using namespace MusicIndexer;

int main() {
  SQLite::Database correct_db{ "/tmp/correct_db.sqlite", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE };
  SQLite::Database testing_db{ "/tmp/testing_db.sqlite", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE };

  MusicIndexer::init_database(correct_db);
  MusicIndexer::init_database(testing_db);

  correct_db.exec("INSERT OR IGNORE INTO t_music_dirs VALUES (NULL, '/home/blackpill0w/Code/c_c++/Music-Indexer/music_for_testing')");
  insert<MusicDir>(testing_db, "/home/blackpill0w/Code/c_c++/Music-Indexer/music_for_testing");
}
