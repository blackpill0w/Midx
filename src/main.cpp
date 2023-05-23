#include <iostream>
#include <SQLiteCpp/SQLiteCpp.h>
#include <taglib/tstring.h>

#include "./dbops.hpp"

namespace MI = MusicIndexer;

int main(int argc, const char *argv[]) {
  std::cout << std::boolalpha;
  SQLite::Database db{"../music_library.sqlite",
                      SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE};
  MI::DBOps::init_database(db);
}
