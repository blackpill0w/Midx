#include "./dbops.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace MusicIndexer::DBOps {

// Static helper functions
namespace Utils {
/**
 * Checks whether a file is of a supported format.
 * Currently only `.flac` and `.mp3` are supported.
 */
bool is_supported_file_type(const std::string& path) {
  constexpr std::array<std::string_view, 2> exts{".flac", ".mp3"};
  return std::ranges::any_of(exts, [&](const auto& ext) { return path.ends_with(ext); });
}
}  // namespace Utils

void init_database(SQLite::Database& db) {
  try {
    db.exec("PRAGMA foreign_keys = ON;");
    // Create music directiries table
    db.exec(R"--(
      CREATE TABLE IF NOT EXISTS t_music_dirs (
        id              INTEGER PRIMARY KEY AUTOINCREMENT,
        path            TEXT NOT NULL UNIQUE
      );
    )--");
    // Create artists table
    db.exec(R"--(
      CREATE TABLE IF NOT EXISTS t_artists (
        id              INTEGER PRIMARY KEY AUTOINCREMENT,
        name            TEXT NOT NULL UNIQUE
      );
    )--");
    // Create albums table
    db.exec(R"--(
      CREATE TABLE IF NOT EXISTS t_albums (
        id                         INTEGER PRIMARY KEY AUTOINCREMENT,
        name                       TEXT NOT NULL,
        artist_id                  INTEGER,
        FOREIGN KEY(artist_id)     REFERENCES t_artists(id),
        CONSTRAINT unique_artist_album UNIQUE (name, artist_id)
      );
    )--");
    // Create tracks table
    db.exec(R"--(
      CREATE TABLE IF NOT EXISTS t_tracks (
        id                         INTEGER PRIMARY KEY AUTOINCREMENT,
        file_path                  TEXT NOT NULL UNIQUE,
        parent_dir_id              INTEGER NOT NULL,
        FOREIGN KEY(parent_dir_id) REFERENCES t_music_dirs(id)
      );
    )--");

    // Create tracks' metadata table
    db.exec(R"--(
      CREATE TABLE IF NOT EXISTS t_tracks_metadata (
        track_id                   INTEGER NOT NULL UNIQUE,
        title                      TEXT,
        track_num                  INTEGER,
        artist_id                  INTEGER,
        album_id                   INTEGER,
        FOREIGN KEY(track_id)      REFERENCES t_tracks(id),
        FOREIGN KEY(artist_id)     REFERENCES t_artists(id),
        FOREIGN KEY(album_id)      REFERENCES t_albums(id),
        CONSTRAINT PK_t_tracks_metadata PRIMARY KEY (track_id)
      );
    )--");
  } catch (SQLite::Exception& e) {
    std::cerr << "Error initialising databases: " << e.what() << "\n";
    std::cerr << "Code: " << e.getErrorCode();
    std::cerr << "Query: " << e.getErrorStr() << "\n";
    exit(1);
  }
}

/**
 * Get all the music directories.
 */
template<>
std::vector<MusicDir> get<MusicDir>(SQLite::Database& db) {
  std::vector<MusicDir> res{};
  SQLite::Statement stmt{db, "SELECT id, path FROM t_music_dirs"};
  while (stmt.executeStep()) {
    std::cout << stmt.getColumn(1).getString() << "\n";
    const int id = stmt.getColumn(0);
    const std::string dir_name{stmt.getColumn(1).getString()};
    res.emplace_back(MusicDir{std::move(dir_name), id});
  }
  return res;
}

/**
 * Get all the artists.
 */
template<>
std::vector<Artist> get<Artist>(SQLite::Database& db) {
  std::vector<Artist> res{};
  SQLite::Statement stmt{db, "SELECT id, name FROM t_artists"};
  while (stmt.executeStep()) {
    const int id = stmt.getColumn(0);
    const std::string artist_name{stmt.getColumn(1).getString()};
    res.emplace_back(Artist{std::move(artist_name), id});
  }
  return res;
}

/**
 * Get all the albums.
 */
template<>
std::vector<Album> get<Album>(SQLite::Database& db) {
  std::vector<Album> res{};
  SQLite::Statement stmt{db, "SELECT id, name, artist_id FROM t_albums"};
  while (stmt.executeStep()) {
    const int id = stmt.getColumn(0);
    const std::string album_name{stmt.getColumn(1).getString()};
    const std::optional<int> artist_id =
        stmt.isColumnNull(2) ? std::nullopt : std::optional<int>{stmt.getColumn(2)};
    res.emplace_back(Album{std::move(album_name), id, artist_id});
  }
  return res;
}

/**
 * Get all the tracks.
 */
template<>
std::vector<Track> get<Track>(SQLite::Database& db) {
  std::vector<Track> res{};

  SQLite::Statement stmt{db, "SELECT id, file_path, parent_dir_id FROM t_tracks"};
  while (stmt.executeStep()) {
    const int id = stmt.getColumn(0);
    const std::string file_path{stmt.getColumn(1).getString()};
    const int parent_dir_id = stmt.getColumn(2);
    res.emplace_back(Track{id, file_path, parent_dir_id});
  }
  return res;
}

template<>
bool is_valid_id<MusicDir>(SQLite::Database& db, const int id) {
  SQLite::Statement stmt{db, "SELECT EXISTS(SELECT 1 FROM t_music_dirs WHERE id = ?)"};
  stmt.bind(1, id);
  stmt.executeStep();
  return stmt.getColumn(0).getInt() == 1;
}

template<>
bool is_valid_id<Artist>(SQLite::Database& db, const int id) {
  SQLite::Statement stmt{db, "SELECT EXISTS(SELECT 1 FROM t_artists WHERE id = ?)"};
  stmt.bind(1, id);
  stmt.executeStep();
  return stmt.getColumn(0).getInt() == 1;
}

template<>
bool is_valid_id<Album>(SQLite::Database& db, const int id) {
  SQLite::Statement stmt{db, "SELECT EXISTS(SELECT 1 FROM t_albums WHERE id = ?)"};
  stmt.bind(1, id);
  stmt.executeStep();
  return stmt.getColumn(0).getInt() == 1;
}

template<>
bool is_valid_id<Track>(SQLite::Database& db, const int id) {
  SQLite::Statement stmt{db, "SELECT EXISTS(SELECT 1 FROM t_tracks WHERE id = ?)"};
  stmt.bind(1, id);
  stmt.executeStep();
  return stmt.getColumn(0).getInt() == 1;
}

/**
 * @param path absolute or relative path to the directory
 */
template<>
std::optional<int> get_id<MusicDir>(SQLite::Database& db, const std::string& path,
                                    [[maybe_unused]] const std::optional<int> dummy) {
  SQLite::Statement stmt{db, "SELECT id FROM t_music_dirs WHERE path = ?"};
  stmt.bindNoCopy(1, path);
  stmt.executeStep();
  return stmt.hasRow() ? std::optional<int>{stmt.getColumn(0)} : std::nullopt;
}

/**
 * @param name name of the artist
 */
template<>
std::optional<int> get_id<Artist>(SQLite::Database& db, const std::string& name,
                                  [[maybe_unused]] const std::optional<int> dummy) {
  SQLite::Statement stmt{db, "SELECT id FROM t_artists WHERE name = ?"};
  stmt.bindNoCopy(1, name);
  stmt.executeStep();
  return stmt.hasRow() ? std::optional<int>{stmt.getColumn(0)} : std::nullopt;
}

/**
 * @param name the name of the album
 * @param artist_id the id of the albums's artist if it exists
 */
template<>
std::optional<int> get_id<Album>(SQLite::Database& db, const std::string& name,
                                 const std::optional<int> artist_id) {
  SQLite::Statement stmt{db, "SELECT id FROM t_albums WHERE name = ? AND artist_id = ?"};
  stmt.bindNoCopy(1, name);
  if (artist_id.has_value())
    stmt.bind(2, artist_id.value());
  else
    stmt.bind(2);
  stmt.executeStep();
  return stmt.hasRow() ? std::optional<int>{stmt.getColumn(0)} : std::nullopt;
}

/**
 * @param file_path path to the track's file
 */
template<>
std::optional<int> get_id<Track>(SQLite::Database& db, const std::string& file_path,
                                 [[maybe_unused]] const std::optional<int> dummy) {
  SQLite::Statement stmt{db, "SELECT id FROM t_tracks WHERE file_path = ?"};
  stmt.bindNoCopy(1, file_path);
  stmt.executeStep();
  return stmt.hasRow() ? std::optional<int>{stmt.getColumn(0)} : std::nullopt;
}

/**
 * Adds a directory to the database. If it already exists in the database,
 * nothing happens.
 *
 * @param path absolute or relative path to the directory.
 *
 * @return the id of the inserted item if everthing goes correctly, otherwise
 * std::nullopt.
 */
template<>
std::optional<int> insert<MusicDir>(SQLite::Database& db, const std::string& path,
                                    [[maybe_unused]] const std::optional<int> dummy) {
  if (not fs::exists(path) or not fs::is_directory(path))
    return std::nullopt;
  const std::string abs_path = fs::canonical(path);
  SQLite::Statement stmt{
      db, "INSERT OR IGNORE INTO t_music_dirs (id, path) VALUES (NULL, ?)"};
  stmt.bindNoCopy(1, abs_path);
  stmt.exec();
  return get_id<MusicDir>(db, abs_path);
}

/**
 * Adds an artist to the database. If it already exists in the database,
 * nothing happens.
 *
 * @param name name of the artist.
 *
 * @return the id of the artist if everthing goes correctly,
 * otherwise std::nullopt.
 */
template<>
std::optional<int> insert<Artist>(SQLite::Database& db, const std::string& name,
                                  [[maybe_unused]] const std::optional<int> dummy) {
  SQLite::Statement stmt{db,
                         "INSERT OR IGNORE INTO t_artists (id, name) VALUES (NULL, ?)"};
  stmt.bindNoCopy(1, name);
  stmt.exec();
  return get_id<Album>(db, name);
}

/**
 * Adds an album to the database. If it already exists in the database,
 * nothing happens.
 *
 * @param name name of the album.
 * @param artist_id id of the album's artist, if std::nullopt is passed the artist
 * will be marked as unknown.
 *
 * @return the id of the album if everthing goes correctly,
 * otherwise std::nullopt.
 */
template<>
std::optional<int> insert<Album>(SQLite::Database& db, const std::string& name,
                                 const std::optional<int> artist_id) {
  if (artist_id.has_value() and not is_valid_id<Artist>(db, artist_id.value())) {
    return std::nullopt;
  }
  SQLite::Statement stmt{
      db, "INSERT OR IGNORE INTO t_albums (id, name, artist_id) VALUES (NULL, ?, ?)"};
  stmt.bindNoCopy(1, name);
  if (artist_id.has_value())
    stmt.bind(2, artist_id.value());
  else
    stmt.bind(2);
  stmt.exec();

  return get_id<Album>(db, name);
}

/**
 * Adds an track to the database. If it already exists in the database,
 * the metadata is updated if needed.
 *
 * @param file_path absolute or relative path to the track's file.
 *
 * @param parent_dir_id id of the parent directory, necessary.
 *
 * @return the id of the track inserted if everthing goes correctly,
 * otherwise std::nullopt.
 */
template<>
std::optional<int> insert<Track>(SQLite::Database& db, const std::string& file_path,
                                 const std::optional<int> parent_dir_id) {
  if (parent_dir_id == std::nullopt) {
    return std::nullopt;
  }
  if (parent_dir_id.has_value() and
      not is_valid_id<MusicDir>(db, parent_dir_id.value())) {
    return std::nullopt;
  }
  const std::string abs_path = fs::canonical(file_path);
  if (not fs::exists(abs_path) or not fs::is_regular_file(abs_path)) {
    return std::nullopt;
  }
  SQLite::Statement stmt{db,
                         "INSERT OR IGNORE INTO t_tracks (id, file_path, parent_dir_id) "
                         "VALUES (NULL, ?, ?)"};
  stmt.bindNoCopy(1, abs_path);
  stmt.bind(2, parent_dir_id.value());
  stmt.exec();
  const std::optional<int> trk_id = get_id<Track>(db, abs_path);
  // Metadata
  // TODO: get_metadata()
  /* TrackMetadata m = get_metadata(db, file_path);
     SQLite::Statement stmt2{ R"--(
      INSERT INTO t_tracks_metadata (track_id, title, track_num, artist_id, album_id)
      VALUES (?, ?, ?, ?, ?);
      )--"};
     stmt2.bind(0, m.track_id)
     stmt2.bind(1, m.title
     stmt2.bind(2, m.track_number
     stmt2.bind(3, m.artist_id
     stmt2.bind(4, m.album_id);
     stmt2.exec();
     */
  return trk_id;
}

std::optional<int> scan_directory(SQLite::Database& db, const std::string& path,
                                  const std::optional<int> parent_dir_id) {
  const std::string abs_path{fs::canonical(path)};
  if (not fs::exists(abs_path) or not fs::is_directory(abs_path)) {
    return std::nullopt;
  }
  const std::optional<int> id =
      parent_dir_id.has_value() ? parent_dir_id : insert<MusicDir>(db, abs_path);
  for (const auto& dir_entry : fs::recursive_directory_iterator(abs_path)) {
    if (dir_entry.is_directory()) {
      scan_directory(db, dir_entry.path(), id);
    }
    else if (dir_entry.is_regular_file()) {
      insert<Track>(db, dir_entry.path(), id);
    }
  }
  return id;
}

}  // namespace MusicIndexer::DBOps
