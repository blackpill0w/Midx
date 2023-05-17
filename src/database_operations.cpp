#include "./database_operations.hpp"

#include <algorithm>
#include <array>
#include <filesystem>

namespace fs = std::filesystem;

namespace MusicIndexer::DatabaseOperations {

// Static helper functions

namespace Utils {
/**
  Checks whether a file is of a supported format.
  Currently only `.flac` and `.mp3` are supported.
*/
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

/**
  Get all the music directories.
*/
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

/**
  Get all the artists.
*/
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

/**
  Get all the albums.
*/
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

/**
  Get all the tracks.
*/
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

/**
  @param path absolute or relative path to the directory
*/
template<>
std::optional<int> get_rowid<MusicDir>(sqlite::database& db, const std::string& path,
                                       [[maybe_unused]] const std::optional<int> dummy) {
  std::optional<int> res{};
  const std::string abs_path = fs::canonical(path);
  try {
    // clang-format off
    db << "SELECT rowid FROM t_music_dirs WHERE path = ?"
       << abs_path
       >> res;
    // clang-format on
  } catch (sqlite::errors::no_rows& e) {
  }
  return res;
}

/**
  @param name name of the artist
*/
template<>
std::optional<int> get_rowid<Artist>(sqlite::database& db, const std::string& name,
                                     [[maybe_unused]] const std::optional<int> dummy) {
  std::optional<int> res{};
  try {
    // clang-format off
    db << "SELECT rowid FROM t_artists WHERE name = ?"
       << name
       >> res;
    // clang-format on
  } catch (sqlite::errors::no_rows& e) {
  }
  return res;
}

/**
  @param name the name of the album
  @param artist_id the rowid of the albums's artist if it exists
*/
template<>
std::optional<int> get_rowid<Album>(sqlite::database& db, const std::string& name,
                                    const std::optional<int> artist_id) {
  std::optional<int> res{};
  try {
    // clang-format off
    db << "SELECT rowid FROM t_albums WHERE name = ? AND artist_id = ?"
       << name << artist_id
       >> res;
    // clang-format on
  } catch (sqlite::errors::no_rows& e) {
  }
  return res;
}

/**
  @param file_path path to the track's file
*/
template<>
std::optional<int> get_rowid<Track>(sqlite::database& db, const std::string& file_path,
                                    [[maybe_unused]] const std::optional<int> dummy) {
  std::optional<int> res{};
  try {
    // clang-format off
    db << "SELECT rowid FROM t_tracks WHERE file_path = ?"
       << file_path
       >> res;
    // clang-format on
  } catch (sqlite::errors::no_rows& e) {
  }
  return res;
}

/**
  Adds a directory to the database. If it already exists in the database,
  nothing happens.

  @param path absolute or relative path to the directory.

  @return the rowid of the inserted item if the directory exists and the given path points
  to a direcory, otherwise std::nullopt.
*/
template<>
std::optional<int> insert<MusicDir>(sqlite::database& db, const std::string& path,
                                    [[maybe_unused]] const std::optional<int> dummy) {
  if (not fs::exists(path) or not fs::is_directory(path)) {
    return std::nullopt;
  }
  const std::string abs_path = fs::canonical(path);
  std::cout << abs_path << "\n";
  db << "INSERT OR IGNORE INTO t_music_dirs VALUES (?)" << abs_path;
  return get_rowid<MusicDir>(db, abs_path);
}

/**
  Adds an artist to the database. If it already exists in the database,
  nothing happens.

  @param name name of the artist.

  @return the rowid of the artist if everthing goes correctly,
  otherwise std::nullopt.
*/
template<>
std::optional<int> insert<Artist>(sqlite::database& db, const std::string& name,
                                  [[maybe_unused]] const std::optional<int> dummy) {
  db << "INSERT OR IGNORE INTO t_artists VALUES (?)" << name;
  return get_rowid<Album>(db, name);
}

/**
  Adds an album to the database. If it already exists in the database,
  nothing happens.

  @param name name of the album.
  @param artist_rowid rowid of the album's artist, if std::nullopt is passed the artist
  will be marked as unknown.

  @return the rowid of the album if everthing goes correctly,
  otherwise std::nullopt.
*/
template<>
std::optional<int> insert<Album>(sqlite::database& db, const std::string& name,
                                 const std::optional<int> artist_rowid) {
  // TODO: make sure artist_rowid is valid
  // clang-format off
  db << "INSERT OR IGNORE INTO t_albums (name, artist_id) VALUES (?, ?)"
     << name << artist_rowid;
  // clang-format on
  return get_rowid<Album>(db, name);
}

/**
  Adds an track to the database. If it already exists in the database,
  the metadata is updated if needed.

  @param file_path absolute or relative path to the track's file.
  @param parent_dir_rowid rowid of the parent directory, necessary.

  @return the rowid of the track if everthing goes correctly,
  otherwise std::nullopt.
*/
template<>
std::optional<int> insert<Track>(
    sqlite::database& db, const std::string& file_path,
    [[maybe_unused]] const std::optional<int> parent_dir_rowid) {
  std::optional<int> res{};
  // TODO: make sure parent_dir_rowid is valid
  if (not fs::exists(file_path) or not fs::is_regular_file(file_path)) {
    return res;
  }
  db << "INSERT OR IGNORE INTO t_tracks (file_path, parent_dir_id) VALUES (?, ?)"
     << file_path << parent_dir_rowid;
  // TODO: Get metadata and insert it (or update if it's already there)
  return get_rowid<Track>(db, file_path);
}

}  // namespace MusicIndexer::DatabaseOperations
