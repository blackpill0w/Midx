#include <iostream>
#include <SQLiteCpp/SQLiteCpp.h>
#include <taglib/fileref.h>

#include "./dbops.hpp"

using namespace MusicIndexer;


int main(int argc, const char *argv[]) {
  SQLite::Database db{"../music_library.sqlite",
                      SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE};
  DBOps::init_database(db);

  DBOps::insert<MusicDir>(db, "/run/media/blackpill0w/Linux Backup/blackpill0w/Music/Death Grips");

  DBOps::build_music_library(db);
}
