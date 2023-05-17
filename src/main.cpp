#include <sqlite_modern_cpp.h>

#include "./database_operations.hpp"

namespace DBOps = MusicIndexer::DatabaseOperations;

void help() {
  std::cout << "COMMANDS:\n"
               "\thelp\tdisplay this message\n"
               "\tinsert <DIRECTORY>\trelative or absolute path\n"
               "\tlist mdirs|artists|albums|tracks\n";
}

int main(int argc, const char *argv[]) {
  sqlite::database db{"../music_library.sqlite"};
  DBOps::init_database(db);

  std::string cmd{};
  std::string arg{};
  std::cout << "Enter 'help' to see the available commands\n";
  while (true) {
    std::cout << "-> ";
    std::cin >> cmd >> arg;
    if (cmd == "q") {
      break;
    }
    else if (cmd == "help") {
      help();
    }
    else if (cmd == "insert") {
      if (arg == "") {
        std::cerr << "ERROR: Missing argument\n";
      }
      else {
        auto rowid = DBOps::insert<MusicIndexer::MusicDir>(db, arg);
        if (rowid == std::nullopt) {
          std::cerr
              << "ERROR: Couldn't insert directory, either it doesn't exist or the given "
                 "path doesn't point to a directory\n";
        }
        else {
          std::cout << "Directory inserted, rowid: " << rowid.value() << "\n";
        }
      }
    }
    else if (cmd == "list") {
      if (arg == "") {
        std::cerr << "Missing argument\n";
      }
      else if (arg == "mdirs") {
        for (const auto& mdir : DBOps::get<MusicIndexer::MusicDir>(db)) {
          std::cout << "\t- " << mdir.get_rowid().value() << ": " << mdir.get_path()
                    << "\n";
        }
      }
      else if (arg == "artists") {
        for (const auto& artist : DBOps::get<MusicIndexer::Artist>(db)) {
          std::cout << "\t- " << artist.get_rowid().value() << ": " << artist.get_name()
                    << "\n";
        }
      }
      else if (arg == "albums") {
        for (const auto& album : DBOps::get<MusicIndexer::Album>(db)) {
          std::cout << "\t- " << album.get_rowid().value() << ": " << album.get_name()
                    << "\n";
        }
      }
      else if (arg == "tracks") {
        for (const auto& track : DBOps::get<MusicIndexer::Track>(db)) {
          std::cout << "\t- " << track.get_rowid().value() << ": "
                    << track.get_parent_dir_rowid().value() << "\n";
        }
      }
      else {
        std::cerr << "Unknown argument\n";
      }
    }
  }
}
