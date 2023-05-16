#include <sqlite_modern_cpp.h>

#include <string>

#include "./cli.hpp"
#include "./database_operations.hpp"

using namespace MusicIndexer::CLI;
namespace DBOps = MusicIndexer::DatabaseOperations;

int main(int argc, const char *argv[]) {
  sqlite::database db{"../music_library.sqlite"};
  DBOps::init_database(db);

  for (const auto& mdir: DBOps::get<MusicIndexer::MusicDir>(db)) {
    std::cout << "- " << mdir.get_path() << "\n";
  };
}
