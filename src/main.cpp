
#include <SQLiteCpp/SQLiteCpp.h>

#include <iostream>
#include <sstream>

#include "./dbops.hpp"

namespace MI = MusicIndexer;

void help() {
  std::cout <<
      R"--(COMMANDS:
  help                                display this message
  insert <DIRECTORY>                  relative or absolute path
  list   mdirs|artists|albums|tracks  list everything
)--";
}

int main(int argc, const char *argv[]) {
  std::cout << std::boolalpha;
  SQLite::Database db{"../music_library.sqlite",
                      SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE};
  MI::DBOps::init_database(db);
  if (std::nullopt == MI::DBOps::scan_directory(db, "../fake_music_for_testing"))
    std::cerr << "WTF!!\n";
}
