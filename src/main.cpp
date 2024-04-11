#include <filesystem>
#include <format>
#include <SQLiteCpp/SQLiteCpp.h>
#include <spdlog/spdlog.h>

#include "./midx.hpp"

using namespace Midx;

int main() {
  Midx::data_dir = "./midx-test";
  std::filesystem::create_directory(Midx::data_dir);

  SQLite::Database db{
      std::format("{}/db.sqlite", Midx::data_dir), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE
  };

  Midx::init_database(db);

  auto mdir_id = insert_music_dir(db, "/home/blackpill0w/Music/");
  if (! mdir_id.has_value()) {
    spdlog::error("ERROR: Failed to insert directory.");
  }

  Midx::build_music_library(db);
}
