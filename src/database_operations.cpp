#include "./database_operations.hpp"

#include <algorithm>
#include <array>
#include <filesystem>

namespace fs = std::filesystem;

namespace MusicIndexer::DatabaseOperations {

// Static helper functions

namespace Utils {
bool is_supported_file_type(const std::string& path) {
  constexpr std::array<std::string_view, 2> exts{".flac", ".mp3"};
  return std::ranges::any_of(exts, [&](const auto& ext) { return path.ends_with(ext); });
}
}  // namespace Utils

void init_database(sqlite::database& db) {
  try {
    db << "PRAGMA foreign_keys = ON;";
    // Create music directiries table
    db << R"--(
      CREATE TABLE IF NOT EXISTS t_music_dirs (
        path               TEXT NOT NULL UNIQUE
      );
    )--";
    // Create artists table
    db << R"--(
      CREATE TABLE IF NOT EXISTS t_artists (
        name            TEXT NOT NULL UNIQUE
      );
    )--";
    // Create albums table
    db << R"--(
      CREATE TABLE IF NOT EXISTS t_albums (
        name                   TEXT NOT NULL,
        artist_id              INTEGER,
        FOREIGN KEY(artist_id)         REFERENCES t_artists(rowid),
        CONSTRAINT unique_artist_album UNIQUE (name, artist_id)
      )
    )--";
    // Create tracks table
    db << R"--(
      CREATE TABLE IF NOT EXISTS t_tracks (
        file_path      TEXT NOT NULL UNIQUE,
        parent_dir_id  INTEGER,
        FOREIGN KEY(parent_dir_id)  REFERENCES t_music_dirs(rowid));
    )--";

    // Create tracks' metadata table
    db << R"--(
      CREATE TABLE IF NOT EXISTS t_tracks_metadata (
        track_id            INTEGER NOT NULL UNIQUE,
        title               TEXT,
        track_num           INTEGER,
        artist_id           INTEGER,
        album_id            INTEGER,
        FOREIGN KEY(track_id)   REFERENCES t_tracks(rowid),
        FOREIGN KEY(artist_id)  REFERENCES t_artists(rowid),
        FOREIGN KEY(album_id)   REFERENCES t_albums(rowid),
        CONSTRAINT PK_t_track_metadata PRIMARY KEY (track_id)
      )
    )--";
  } catch (sqlite::sqlite_exception& e) {
    std::cerr << "Error initialising databases: " << e.what() << "\n";
    std::cerr << "Query: " << e.get_sql() << "\n";
    exit(1);
  }
}

template<>
std::vector<MusicDir> get<MusicDir>(sqlite::database& db) {
  std::vector<MusicDir> res{};
  for (auto&& row : db << "SELECT rowid, * FROM t_music_dirs") {
    int rowid{};
    std::string dir_name{};
    row >> rowid >> dir_name;
    res.emplace_back(MusicDir{std::move(dir_name), rowid});
  }
  return res;
}

template<>
std::vector<Artist> get<Artist>(sqlite::database& db) {
  std::vector<Artist> res{};
  for (auto&& row : db << "SELECT rowid, * FROM t_artists") {
    int rowid{};
    std::string artist_name{};
    row >> rowid >> artist_name;
    res.emplace_back(Artist{std::move(artist_name), rowid});
  }
  return res;
}

template<>
std::vector<Album> get<Album>(sqlite::database& db) {
  std::vector<Album> res{};
  for (auto&& row : db << "SELECT rowid, * FROM t_albums") {
    int rowid{};
    std::string album_name{};
    std::optional<int> artist_rowid{};
    row >> rowid >> album_name >> artist_rowid;
    res.emplace_back(Album{std::move(album_name), rowid, artist_rowid});
  }
  return res;
}

template<>
std::vector<Track> get<Track>(sqlite::database& db) {
  std::vector<Track> res{};
  for (auto&& row : db << "SELECT rowid, * FROM t_tracks") {
    int rowid{};
    std::string track_file_path{};
    int parent_dir_rowid{};
    row >> rowid >> track_file_path >> parent_dir_rowid;
    res.emplace_back(Track{std::move(track_file_path), rowid, parent_dir_rowid});
  }
  return res;
}

bool add_music_directory(const std::string& path) {
  if (not (fs::exists(path) and fs::is_directory(path))) {
    return false;
  }
  for (const auto& dir_entry : fs::recursive_directory_iterator(path)) {
    if (dir_entry.is_directory()) {
      add_music_directory(dir_entry.path());
    }
    else if ((fs::is_regular_file(path) or fs::is_symlink(path)) and
             Utils::is_supported_file_type(path)) {
      // add_track();
    }
  }

  return true;
}

}  // namespace MusicIndexer::DatabaseOperations
