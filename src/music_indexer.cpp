#include "./music_indexer.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <ranges>
#include <string>
#include <vector>

#include <SQLiteCpp/SQLiteCpp.h>

#include <taglib/fileref.h>
#include <taglib/flacfile.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>

namespace fs = std::filesystem;

namespace MusicIndexer {

// Static helper functions
namespace Utils {

/**
 * Checks whether a file is of a supported format.
 * Currently only `.flac` and `.mp3` are supported.
 */
static bool is_supported_file_type(const std::string &path);

/**
 * Get metadata from a file.
 */
static std::optional<TrackMetadata> load_metadata(SQLite::Database &db, const int track_id,
                                                  const std::string &file_path);

/**
 * Extract album art from FLAC file.
 */
static std::optional<TagLib::ByteVector> get_flac_album_art(const std::string &filename);

/**
 * Extract album art from MP3 file.
 */
static std::optional<TagLib::ByteVector> get_mp3_album_art(const std::string &filename);

/**
 * Extract album art from tags.
 */
static std::optional<TagLib::ByteVector> get_album_art(const std::string &filename);

/**
 * Insert a TrackMetadata object into the database
 */
static std::optional<int> insert_metadata(SQLite::Database &db, const TrackMetadata &tm);

}  // namespace Utils

void init_database(SQLite::Database &db) {
  if (not fs::exists(data_dir)) {
    fs::create_directory(data_dir);
  }
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
        track_id                   INTEGER PRIMARY KEY,
        title                      TEXT NOT NULL,
        track_num                  INTEGER,
        artist_id                  INTEGER,
        album_id                   INTEGER,
        FOREIGN KEY(track_id)      REFERENCES t_tracks(id),
        FOREIGN KEY(artist_id)     REFERENCES t_artists(id),
        FOREIGN KEY(album_id)      REFERENCES t_albums(id)
      );
    )--");
  } catch (SQLite::Exception &e) {
    std::cerr << "Error initialising the databases: " << e.what() << "\n";
    std::cerr << "Code: " << e.getErrorCode();
    std::cerr << "Query: " << e.getErrorStr() << "\n";
    exit(1);
  }
}

/**
 * Get all the music directories.
 */
template<>
std::vector<MusicDir> get_all<MusicDir>(SQLite::Database &db) {
  std::vector<MusicDir> res{};
  SQLite::Statement stmt{db, "SELECT id, path FROM t_music_dirs"};
  while (stmt.executeStep()) {
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
std::vector<Artist> get_all<Artist>(SQLite::Database &db) {
  std::vector<Artist> res{};
  SQLite::Statement stmt{db, "SELECT id, name FROM t_artists"};
  while (stmt.executeStep()) {
    const int id = stmt.getColumn(0);
    const std::string artist_name{stmt.getColumn(1).getString()};
    res.emplace_back(Artist{id, std::move(artist_name)});
  }
  return res;
}

/**
 * Get all the albums.
 */
template<>
std::vector<Album> get_all<Album>(SQLite::Database &db) {
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
 * Get all the tracks and their metadata if it exists.
 */
template<>
std::vector<Track> get_all<Track>(SQLite::Database &db) {
  std::vector<Track> res{};

  SQLite::Statement stmt{db, R"--(
    SELECT id, file_path, parent_dir_id, title, track_num, artist_id, album_id
    FROM t_tracks t
    LEFT JOIN t_tracks_metadata tm ON t.id = tm.track_id
  )--"};
  while (stmt.executeStep()) {
    const int id = stmt.getColumn(0);
    const std::string file_path{stmt.getColumn(1).getString()};
    const int parent_dir_id = stmt.getColumn(2);
    res.emplace_back(Track{id, file_path, parent_dir_id});
    // Get metadata
    auto title = stmt.getColumn(3).getString();
    auto track_num =
        stmt.isColumnNull(4) ? std::nullopt : std::optional<int>(stmt.getColumn(4).getInt());
    auto artist_id =
        stmt.isColumnNull(5) ? std::nullopt : std::optional<int>(stmt.getColumn(5).getInt());
    auto album_id =
        stmt.isColumnNull(6) ? std::nullopt : std::optional<int>(stmt.getColumn(6).getInt());

    res.back().update_metadata(TrackMetadata{id, std::move(title), track_num, artist_id, album_id});
  }
  return res;
}

template<>
std::optional<Artist> get(SQLite::Database &db, const int id) {
  SQLite::Statement stmt{db, "SELECT id, name FROM t_artists WHERE id = ?"};
  stmt.bind(1, id);
  if (not stmt.executeStep()) {
    return std::nullopt;
  }
  return Artist{stmt.getColumn(0).getInt(), stmt.getColumn(1).getString()};
}

template<>
std::optional<Album> get(SQLite::Database &db, const int id) {
  SQLite::Statement stmt{db, "SELECT id, name, artist_id FROM t_albums WHERE id = ?"};
  stmt.bind(1, id);
  if (not stmt.executeStep()) {
    return std::nullopt;
  }
  return Album{
      stmt.getColumn(1).getString(), stmt.getColumn(0).getInt(),
      stmt.isColumnNull(2) ? std::nullopt : std::optional<int>{stmt.getColumn(2).getInt()}};
}

template<>
std::optional<TrackMetadata> get(SQLite::Database &db, const int id) {
  SQLite::Statement stmt{
      db,
      "SELECT track_id, title, track_num, artist_id, album_id FROM t_tracks_metadata WHERE id = ?"};
  stmt.bind(1, id);
  if (not stmt.executeStep()) {
    return std::nullopt;
  }
  return TrackMetadata{
      stmt.getColumn(0).getInt(), stmt.getColumn(1).getString(),
      stmt.isColumnNull(2) ? std::nullopt : std::optional<int>{stmt.getColumn(2).getInt()},
      stmt.isColumnNull(3) ? std::nullopt : std::optional<int>{stmt.getColumn(3).getInt()},
      stmt.isColumnNull(4) ? std::nullopt : std::optional<int>{stmt.getColumn(4).getInt()}};
}

template<>
bool is_valid_id<MusicDir>(SQLite::Database &db, const int id) {
  SQLite::Statement stmt{db, "SELECT EXISTS(SELECT 1 FROM t_music_dirs WHERE id = ?)"};
  stmt.bind(1, id);
  stmt.executeStep();
  return stmt.getColumn(0).getInt() == 1;
}

template<>
bool is_valid_id<Artist>(SQLite::Database &db, const int id) {
  SQLite::Statement stmt{db, "SELECT EXISTS(SELECT 1 FROM t_artists WHERE id = ?)"};
  stmt.bind(1, id);
  stmt.executeStep();
  return stmt.getColumn(0).getInt() == 1;
}

template<>
bool is_valid_id<Album>(SQLite::Database &db, const int id) {
  SQLite::Statement stmt{db, "SELECT EXISTS(SELECT 1 FROM t_albums WHERE id = ?)"};
  stmt.bind(1, id);
  stmt.executeStep();
  return stmt.getColumn(0).getInt() == 1;
}

template<>
bool is_valid_id<Track>(SQLite::Database &db, const int id) {
  SQLite::Statement stmt{db, "SELECT EXISTS(SELECT 1 FROM t_tracks WHERE id = ?)"};
  stmt.bind(1, id);
  stmt.executeStep();
  return stmt.getColumn(0).getInt() == 1;
}

/**
 * @param path absolute or relative path to the directory
 */
template<>
std::optional<int> get_id<MusicDir>(SQLite::Database &db, const std::string &path,
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
std::optional<int> get_id<Artist>(SQLite::Database &db, const std::string &name,
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
std::optional<int> get_id<Album>(SQLite::Database &db, const std::string &name,
                                 const std::optional<int> artist_id) {
  SQLite::Statement stmt{db, "SELECT id FROM t_albums WHERE name = ? AND artist_id = ?"};
  stmt.bindNoCopy(1, name);
  if (artist_id.has_value()) {
    stmt.bind(2, artist_id.value());
  }
  else
    stmt.bind(2);
  stmt.executeStep();
  return stmt.hasRow() ? std::optional<int>{stmt.getColumn(0).getInt()} : std::nullopt;
}

/**
 * @param file_path path to the track's file
 */
template<>
std::optional<int> get_id<Track>(SQLite::Database &db, const std::string &file_path,
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
std::optional<int> insert<MusicDir>(SQLite::Database &db, const std::string &path,
                                    [[maybe_unused]] const std::optional<int> dummy) {
  if (not fs::exists(path) or not fs::is_directory(path))
    return std::nullopt;
  const std::string abs_path = fs::canonical(path);
  SQLite::Statement stmt{db, "INSERT OR IGNORE INTO t_music_dirs (id, path) VALUES (NULL, ?)"};
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
std::optional<int> insert<Artist>(SQLite::Database &db, const std::string &name,
                                  [[maybe_unused]] const std::optional<int> dummy) {
  SQLite::Statement stmt{db, "INSERT OR IGNORE INTO t_artists (id, name) VALUES (NULL, ?)"};
  stmt.bindNoCopy(1, name);
  stmt.exec();
  return get_id<Artist>(db, name);
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
std::optional<int> insert<Album>(SQLite::Database &db, const std::string &name,
                                 const std::optional<int> artist_id) {
  if (artist_id.has_value() and not is_valid_id<Artist>(db, artist_id.value()))
    return std::nullopt;
  SQLite::Statement stmt{
      db, "INSERT OR IGNORE INTO t_albums (id, name, artist_id) VALUES (NULL, ?, ?)"};
  stmt.bindNoCopy(1, name);
  if (artist_id.has_value())
    stmt.bind(2, artist_id.value());
  else
    stmt.bind(2);
  stmt.exec();

  return get_id<Album>(db, name, artist_id);
}

/**
 * Adds a track to the database. If it already exists, the metadata is updated if needed.
 *
 * @param file_path absolute or relative path to the track's file.
 *
 * @param parent_dir_id id of the parent directory, necessary.
 *
 * @return the id of the track inserted if everthing goes correctly,
 * otherwise std::nullopt.
 */
template<>
std::optional<int> insert<Track>(SQLite::Database &db, const std::string &file_path,
                                 const std::optional<int> parent_dir_id) {
  if (not parent_dir_id or not is_valid_id<MusicDir>(db, parent_dir_id.value())) {
    return std::nullopt;
  }
  const std::string abs_path = fs::canonical(file_path);
  if (not fs::exists(abs_path) or not fs::is_regular_file(abs_path)) {
    return std::nullopt;
  }
  SQLite::Statement stmt{
      db, "INSERT OR IGNORE INTO t_tracks (id, file_path, parent_dir_id) VALUES (NULL, ?, ?)"};
  stmt.bindNoCopy(1, abs_path);
  stmt.bind(2, parent_dir_id.value());
  stmt.exec();
  const std::optional<int> trk_id = get_id<Track>(db, abs_path);
  // Metadata
  std::optional<TrackMetadata> tm = Utils::load_metadata(db, trk_id.value(), file_path);
  if (tm.has_value())
    Utils::insert_metadata(db, tm.value());

  return trk_id;
}

bool remove_track(SQLite::Database &db, const int track_id) {
  SQLite::Statement del_metadata_stmt{db, "DELETE FROM t_tracks_metadata WHERE track_id = ?"};
  SQLite::Statement stmt{db, "DELETE FROM t_tracks WHERE id = ?"};

  del_metadata_stmt.bind(1, track_id);
  stmt.bind(1, track_id);

  del_metadata_stmt.exec();
  stmt.exec();

  return true;
}

std::vector<int> get_ids_of_tracks_of_music_dir(SQLite::Database &db, const int mdir_id) {
  std::vector<int> res{};
  SQLite::Statement stmt{db, R"--(
    SELECT t_tracks.id FROM t_tracks
    JOIN t_music_dirs ON t_tracks.parent_dir_id = t_music_dirs.id
    WHERE t_music_dirs.id = ?;
  )--"};
  stmt.bind(1, mdir_id);
  while (stmt.executeStep()) {
    res.push_back(stmt.getColumn(0).getInt());
  }
  return res;
}

bool remove_music_dir(SQLite::Database &db, const std::string &path) {
  // TODO: return false if path is not in database
  if (not fs::exists(path) or not fs::is_directory(path))
    return false;
  const std::string abs_path      = fs::canonical(path);
  const std::optional<int> dir_id = get_id<MusicDir>(db, abs_path);

  SQLite::Statement del_tracks_metadata_stmt = {db, R"--(
    DELETE FROM t_tracks_metadata
    WHERE track_id in (
      SELECT track_id FROM t_tracks_metadata
      JOIN t_tracks ON t_tracks.id = t_tracks_metadata.track_id
      JOIN t_music_dirs ON t_music_dirs.id = t_tracks.parent_dir_id
      WHERE t_music_dirs.id = ?)
  )--"};
  del_tracks_metadata_stmt.bind(1, dir_id.value());

  SQLite::Statement del_tracks_stmt = {db, R"--(
    DELETE FROM t_tracks
    WHERE t_tracks.id IN (
      SELECT t_tracks.id FROM t_tracks
      JOIN t_music_dirs ON t_music_dirs.id = t_tracks.parent_dir_id
      WHERE t_music_dirs.id = ?)
  )--"};
  del_tracks_stmt.bind(1, dir_id.value());

  SQLite::Statement stmt{db, "DELETE FROM t_music_dirs WHERE id = ?"};
  stmt.bind(1, dir_id.value());

  del_tracks_metadata_stmt.exec();
  del_tracks_stmt.exec();
  stmt.exec();
  return true;
}

std::optional<int> scan_directory(SQLite::Database &db, const std::string &path) {
  const std::string abs_path{fs::canonical(path)};
  if (not fs::exists(abs_path) or not fs::is_directory(abs_path)) {
    return std::nullopt;
  }
  const std::optional<int> id = insert<MusicDir>(db, abs_path);
  int i                       = 1;
  for (const auto &dir_entry : fs::recursive_directory_iterator(abs_path)) {
    if (dir_entry.is_regular_file() && Utils::is_supported_file_type(dir_entry.path())) {
      insert<Track>(db, dir_entry.path(), id);
      std::cout << i << " - INSERTED: " << dir_entry.path() << "\n";
      ++i;
    }
  }
  return id;
}

void build_music_library(SQLite::Database &db) {
  const auto mdirs = get_all<MusicDir>(db);
  for (const auto &mdir : mdirs)
    scan_directory(db, mdir.path);
}

/******************************************************************************/
/************************** --| Static Functions |-- **************************/
/******************************************************************************/

static bool Utils::is_supported_file_type(const std::string &path) {
  static constexpr std::array<std::string_view, 2> exts{".flac", ".mp3"};
  return std::ranges::any_of(exts, [&](const auto &ext) { return path.ends_with(ext); });
}

static std::optional<TrackMetadata> Utils::load_metadata(SQLite::Database &db, const int track_id,
                                                         const std::string &file_path) {
  if (not is_valid_id<Track>(db, track_id))
    return std::nullopt;
  TagLib::FileRef fref{file_path.c_str()};
  if (fref.isNull() or fref.tag()->isEmpty())
    return std::nullopt;

  std::string title;
  if (not fref.tag()->title().isEmpty())
    title = fref.tag()->title().to8Bit(true);
  else
    title = fs::path{file_path}.filename().replace_extension("");

  std::optional<int> trk_num = fref.tag()->track();
  if (trk_num.value() == 0)
    trk_num = std::nullopt;

  std::optional<int> artist_id = std::nullopt;
  if (not fref.tag()->artist().isEmpty()) {
    const std::string artist = fref.tag()->artist().to8Bit(true);
    artist_id                = insert<Artist>(db, artist);
  }

  std::optional<int> album_id = std::nullopt;
  if (not fref.tag()->album().isEmpty()) {
    const std::string album = fref.tag()->album().to8Bit(true);

    album_id = insert<Album>(db, album, artist_id);
  }
  // Extract album art
  const std::string album_art_filename = data_dir + "/" + std::to_string(album_id.value());
  if (album_id.has_value() and not fs::exists(album_art_filename)) {
    std::optional<TagLib::ByteVector> pic = get_album_art(file_path);
    if (pic.has_value()) {
      std::ofstream album_art_file{album_art_filename};
      for (auto b : pic.value()) {
        album_art_file << b;
      }
    }
  }

  return TrackMetadata(track_id, title, trk_num, artist_id, album_id);
}

static std::optional<TagLib::ByteVector> Utils::get_flac_album_art(const std::string &filename) {
  if (not is_supported_file_type(filename) or not filename.ends_with(".flac"))
    return std::nullopt;

  TagLib::FLAC::File f{filename.c_str()};
  if (not f.isValid() or f.pictureList().isEmpty())
    return std::nullopt;
  return f.pictureList().front()->data();
}

static std::optional<TagLib::ByteVector> Utils::get_mp3_album_art(const std::string &filename) {
  if (not is_supported_file_type(filename) or not filename.ends_with(".mp3"))
    return std::nullopt;

  TagLib::MPEG::File f{filename.c_str()};
  if (not f.isValid() or not f.hasID3v2Tag())
    return std::nullopt;
  auto *tags      = f.ID3v2Tag();
  auto &framelist = tags->frameList("APIC");
  if (framelist.isEmpty())
    return std::nullopt;
  auto *pic = static_cast<TagLib::ID3v2::AttachedPictureFrame *>(framelist.front());
  return pic->picture();
}

static std::optional<TagLib::ByteVector> Utils::get_album_art(const std::string &filename) {
  if (not is_supported_file_type(filename))
    return std::nullopt;
  else if (filename.ends_with(".flac"))
    return get_flac_album_art(filename);
  else  // .mp3
    return get_mp3_album_art(filename);
}

static std::optional<int> Utils::insert_metadata(SQLite::Database &db, const TrackMetadata &tm) {
  SQLite::Statement stmt{db, R"--(
      INSERT OR REPLACE INTO t_tracks_metadata (track_id, title, track_num, artist_id, album_id)
      VALUES (?, ?, ?, ?, ?);
  )--"};
  stmt.bind(1, tm.track_id);
  if (tm.title.empty())
    stmt.bind(2);
  else
    stmt.bindNoCopy(2, tm.title);

  if (tm.track_number.has_value())
    stmt.bind(3, tm.track_number.value());
  else
    stmt.bind(3);

  if (tm.artist_id.has_value())
    stmt.bind(4, tm.artist_id.value());
  else
    stmt.bind(4);

  if (tm.album_id.has_value())
    stmt.bind(5, tm.album_id.value());
  else
    stmt.bind(5);

  stmt.exec();

  return tm.track_id;
}

}  // namespace MusicIndexer
