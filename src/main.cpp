#include <SQLiteCpp/SQLiteCpp.h>
#include <filesystem>
#include <format>
#include <spdlog/spdlog.h>

#include "./midx.hpp"

using namespace Midx;

int main() {
  Midx::data_dir = "./midx-test";
  std::filesystem::create_directory(Midx::data_dir);

  SQLite::Database db{std::format("{}/db.sqlite", Midx::data_dir),
                      SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE};

  Midx::init_database(db);

  auto mdir_id = insert<MusicDir>(db, "/home/blackpill0w/Music/Aphex Twin/Drukqs [2001]");
  if (! mdir_id.has_value()) {
    spdlog::error("ERROR: Failed to insert directory.");
  }

  Midx::build_music_library(db);
}
