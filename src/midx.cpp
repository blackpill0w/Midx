#include "./midx.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <vector>

#include <spdlog/spdlog.h>

#include <SQLiteCpp/SQLiteCpp.h>

#include <taglib/fileref.h>
#include <taglib/flacfile.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>

namespace fs = std::filesystem;

using std::nullopt;
using std::optional;
using std::string;
using std::vector;

namespace Midx {

// Static helper functions
namespace Utils {

/**
 * Checks whether a file is of a supported format.
 * Currently only `.flac` and `.mp3` are supported.
 */
static bool is_supported_file_type(const string &path);

/**
 * Get metadata from a file.
 */
static optional<TrackMetadata> load_metadata(
    SQLite::Database &db, const TrackId track_id, const string &file_path
);

/**
 * Extract album art from FLAC file.
 */
static optional<TagLib::ByteVector> get_flac_album_art(const string &filename);

/**
 * Extract album art from MP3 file.
 */
static optional<TagLib::ByteVector> get_mp3_album_art(const string &filename);

/**
 * Extract album art from tags.
 */
static optional<TagLib::ByteVector> get_album_art(const string &filename);

/**
 * Insert a TrackMetadata object into the database
 */
static optional<TrackId> insert_metadata(SQLite::Database &db, const TrackMetadata &tm);

}  // namespace Utils

void init_database(SQLite::Database &db) {
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
    spdlog::error("Error initialising the databases: {}", e.what());
    spdlog::error("Code: {}", e.getErrorCode());
    spdlog::error("Query: {}", e.getErrorStr());
    exit(1);
  }
}

/**
 * Get all the music directories.
 */
vector<MusicDir> get_all_music_dirs(SQLite::Database &db) {
  vector<MusicDir> res{};
  SQLite::Statement stmt{db, "SELECT id, path FROM t_music_dirs"};
  while (stmt.executeStep()) {
    const MDirId id = stmt.getColumn(0).getUInt();
    const string dir_name{stmt.getColumn(1).getString()};
    res.emplace_back(MusicDir{id, std::move(dir_name)});
  }
  return res;
}

/**
 * Get all the artists.
 */
vector<Artist> get_all_artists(SQLite::Database &db) {
  vector<Artist> res{};
  SQLite::Statement stmt{db, "SELECT id, name FROM t_artists"};
  while (stmt.executeStep()) {
    const ArtistId id = stmt.getColumn(0).getUInt();
    const string artist_name{stmt.getColumn(1).getString()};
    res.emplace_back(Artist{id, std::move(artist_name)});
  }
  return res;
}

/**
 * Get all the albums.
 */
vector<Album> get_all_albums(SQLite::Database &db) {
  vector<Album> res{};
  SQLite::Statement stmt{db, "SELECT id, name, artist_id FROM t_albums"};
  while (stmt.executeStep()) {
    const AlbumId id = stmt.getColumn(0).getUInt();
    const string album_name{stmt.getColumn(1).getString()};
    const optional<ArtistId> artist_id =
        stmt.isColumnNull(2) ? optional<ArtistId>{nullopt} : stmt.getColumn(2).getUInt();

    res.emplace_back(Album{id, std::move(album_name), artist_id});
  }
  return res;
}

/**
 * Get all the tracks and their metadata if it exists.
 */
vector<Track> get_all_tracks(SQLite::Database &db) {
  vector<Track> res{};

  SQLite::Statement stmt{db, R"--(
    SELECT id, file_path, parent_dir_id, title, track_num, artist_id, album_id
    FROM t_tracks t
    LEFT JOIN t_tracks_metadata tm ON t.id = tm.track_id
  )--"};
  while (stmt.executeStep()) {
    const TrackId id = stmt.getColumn(0).getUInt();
    const string file_path{stmt.getColumn(1).getString()};
    const MDirId parent_dir_id = stmt.getColumn(2).getUInt();
    res.emplace_back(Track{id, file_path, parent_dir_id});
    // Get metadata
    auto title     = stmt.getColumn(3).getString();
    auto track_num = stmt.isColumnNull(4) ? nullopt : optional<size_t>(stmt.getColumn(4).getUInt());
    stmt.getColumn(4);
    optional<ArtistId> artist_id =
        stmt.isColumnNull(5) ? nullopt : optional<ArtistId>(stmt.getColumn(5).getUInt());
    optional<AlbumId> album_id =
        stmt.isColumnNull(6) ? nullopt : optional<AlbumId>(stmt.getColumn(6).getUInt());

    res.back().update_metadata(TrackMetadata{id, std::move(title), track_num, artist_id, album_id});
  }
  return res;
}

optional<Artist> get_artist(SQLite::Database &db, const ArtistId id) {
  SQLite::Statement stmt{db, "SELECT id, name FROM t_artists WHERE id = ?"};
  stmt.bind(1, uint32_t(id));
  if (not stmt.executeStep()) {
    return nullopt;
  }
  return Artist{stmt.getColumn(0).getUInt(), stmt.getColumn(1).getString()};
}

optional<Album> get_album(SQLite::Database &db, const AlbumId id) {
  SQLite::Statement stmt{db, "SELECT id, name, artist_id FROM t_albums WHERE id = ?"};
  stmt.bind(1, uint32_t(id));
  if (not stmt.executeStep()) {
    return nullopt;
  }
  return Album{
      stmt.getColumn(0).getUInt(), stmt.getColumn(1).getString(),
      stmt.isColumnNull(2) ? nullopt : optional<AlbumId>{stmt.getColumn(2).getUInt()}
  };
}

optional<TrackMetadata> get_track_metadata(SQLite::Database &db, const TrackId id) {
  SQLite::Statement stmt{
      db,
      "SELECT track_id, title, track_num, artist_id, album_id FROM t_tracks_metadata WHERE id = ?"
  };
  stmt.bind(1, uint32_t(id));
  if (not stmt.executeStep()) {
    return nullopt;
  }
  return TrackMetadata{
      stmt.getColumn(0).getUInt(), stmt.getColumn(1).getString(),
      stmt.isColumnNull(2) ? nullopt : optional<size_t>{stmt.getColumn(2).getUInt()},
      stmt.isColumnNull(3) ? nullopt : optional<ArtistId>{stmt.getColumn(3).getUInt()},
      stmt.isColumnNull(4) ? nullopt : optional<AlbumId>{stmt.getColumn(4).getUInt()}
  };
}

bool is_valid_music_dir_id(SQLite::Database &db, const MDirId id) {
  SQLite::Statement stmt{db, "SELECT EXISTS(SELECT 1 FROM t_music_dirs WHERE id = ?)"};
  stmt.bind(1, uint32_t(id));
  stmt.executeStep();
  return stmt.getColumn(0).getInt() == 1;
}

bool is_valid_artist_id(SQLite::Database &db, const ArtistId id) {
  SQLite::Statement stmt{db, "SELECT EXISTS(SELECT 1 FROM t_artists WHERE id = ?)"};
  stmt.bind(1, uint32_t(id));
  stmt.executeStep();
  return stmt.getColumn(0).getInt() == 1;
}

bool is_valid_album_id(SQLite::Database &db, const AlbumId id) {
  SQLite::Statement stmt{db, "SELECT EXISTS(SELECT 1 FROM t_albums WHERE id = ?)"};
  stmt.bind(1, uint32_t(id));
  stmt.executeStep();
  return stmt.getColumn(0).getInt() == 1;
}

bool is_valid_track_id(SQLite::Database &db, const TrackId id) {
  SQLite::Statement stmt{db, "SELECT EXISTS(SELECT 1 FROM t_tracks WHERE id = ?)"};
  stmt.bind(1, uint32_t(id));
  stmt.executeStep();
  return stmt.getColumn(0).getInt() == 1;
}

optional<MDirId> get_music_dir_id(SQLite::Database &db, const string &path) {
  SQLite::Statement stmt{db, "SELECT id FROM t_music_dirs WHERE path = ?"};
  stmt.bindNoCopy(1, path);
  stmt.executeStep();
  return stmt.hasRow() ? optional<MDirId>{stmt.getColumn(0).getUInt()} : nullopt;
}

optional<ArtistId> get_artist_id(SQLite::Database &db, const string &name) {
  SQLite::Statement stmt{db, "SELECT id FROM t_artists WHERE name = ?"};
  stmt.bindNoCopy(1, name);
  stmt.executeStep();
  return stmt.hasRow() ? optional<ArtistId>{stmt.getColumn(0).getUInt()} : nullopt;
}

optional<AlbumId> get_album_id(
    SQLite::Database &db, const string &name, const optional<ArtistId> artist_id
) {
  SQLite::Statement stmt{db, "SELECT id FROM t_albums WHERE name = ? AND artist_id = ?"};
  stmt.bindNoCopy(1, name);
  if (artist_id.has_value()) {
    stmt.bind(2, uint32_t(*artist_id));
  } else
    stmt.bind(2);
  stmt.executeStep();
  return stmt.hasRow() ? optional<AlbumId>{stmt.getColumn(0).getUInt()} : nullopt;
}

optional<TrackId> get_track_id(SQLite::Database &db, const string &file_path) {
  SQLite::Statement stmt{db, "SELECT id FROM t_tracks WHERE file_path = ?"};
  const auto abs_path = fs::canonical(file_path);
  if (not abs_path.has_filename()) {
    spdlog::error("File does not exist: {}", abs_path.c_str());
    return nullopt;
  }
  stmt.bindNoCopy(1, abs_path.c_str());
  stmt.executeStep();
  return stmt.hasRow() ? optional<TrackId>{stmt.getColumn(0).getUInt()} : nullopt;
}

optional<MDirId> insert_music_dir(SQLite::Database &db, const string &path) {
  if (not fs::exists(path) or not fs::is_directory(path)) {
    spdlog::error("Path doesn't exists or is not a directory: {}", path);
    return nullopt;
  }
  const string abs_path = fs::canonical(path);
  const auto id         = get_music_dir_id(db, abs_path);
  if (id.has_value()) {
    return id;
  }
  SQLite::Statement stmt{db, "INSERT OR IGNORE INTO t_music_dirs (id, path) VALUES (NULL, ?)"};
  stmt.bindNoCopy(1, abs_path);
  stmt.exec();
  return get_music_dir_id(db, abs_path);
}

optional<ArtistId> insert_artist(SQLite::Database &db, const string &name) {
  const auto id = get_artist_id(db, name);
  if (id.has_value()) {
    return id;
  }
  SQLite::Statement stmt{db, "INSERT OR IGNORE INTO t_artists (id, name) VALUES (NULL, ?)"};
  stmt.bindNoCopy(1, name);
  stmt.exec();
  return get_artist_id(db, name);
}

optional<AlbumId> insert_album(
    SQLite::Database &db, const string &name, const optional<ArtistId> artist_id
) {
  if (artist_id.has_value() and not is_valid_artist_id(db, *artist_id)) {
    return nullopt;
  }
  const auto id = get_album_id(db, name, artist_id);
  if (id.has_value()) {
    return id;
  }
  SQLite::Statement stmt{
      db, "INSERT OR IGNORE INTO t_albums (id, name, artist_id) VALUES (NULL, ?, ?)"
  };
  stmt.bindNoCopy(1, name);
  if (artist_id.has_value())
    stmt.bind(2, uint32_t(*artist_id));
  else
    stmt.bind(2);
  stmt.exec();

  return get_album_id(db, name, artist_id);
}

optional<TrackId> insert_track(
    SQLite::Database &db, const string &file_path, const optional<MDirId> parent_dir_id
) {
  if (not parent_dir_id or not is_valid_music_dir_id(db, *parent_dir_id)) {
    return nullopt;
  }
  const string abs_path = fs::canonical(file_path);
  if (not fs::exists(abs_path) or not fs::is_regular_file(abs_path)) {
    spdlog::error("Path doesn't exists or is not a directory: {}", abs_path);
    return nullopt;
  }
  const auto id = get_track_id(db, abs_path);
  if (id.has_value()) {
    return id;
  }
  SQLite::Statement stmt{
      db, "INSERT OR IGNORE INTO t_tracks (id, file_path, parent_dir_id) VALUES (NULL, ?, ?)"
  };
  stmt.bindNoCopy(1, abs_path);
  stmt.bind(2, uint32_t(*parent_dir_id));
  stmt.exec();
  const optional<TrackId> trk_id = get_track_id(db, abs_path);
  // Metadata
  optional<TrackMetadata> tm = Utils::load_metadata(db, *trk_id, file_path);
  if (tm.has_value())
    Utils::insert_metadata(db, tm.value());

  return trk_id;
}

bool remove_track(SQLite::Database &db, const TrackId track_id) {
  SQLite::Statement del_metadata_stmt{db, "DELETE FROM t_tracks_metadata WHERE track_id = ?"};
  SQLite::Statement stmt{db, "DELETE FROM t_tracks WHERE id = ?"};

  del_metadata_stmt.bind(1, uint32_t(track_id));
  stmt.bind(1, uint32_t(track_id));

  del_metadata_stmt.exec();
  stmt.exec();

  return true;
}

vector<TrackId> get_ids_of_tracks_of_music_dir(SQLite::Database &db, const MDirId mdir_id) {
  vector<TrackId> res{};
  SQLite::Statement stmt{db, R"--(
    SELECT t_tracks.id FROM t_tracks
    JOIN t_music_dirs ON t_tracks.parent_dir_id = t_music_dirs.id
    WHERE t_music_dirs.id = ?;
  )--"};
  stmt.bind(1, uint32_t(mdir_id));
  while (stmt.executeStep()) {
    res.push_back(stmt.getColumn(0).getUInt());
  }
  return res;
}

bool remove_music_dir(SQLite::Database &db, const string &path) {
  if (not fs::exists(path) or not fs::is_directory(path)) {
    spdlog::error("Path doesn't exists or is not a directory: {}", path);
    return false;
  }
  const string abs_path         = fs::canonical(path);
  const optional<MDirId> dir_id = get_music_dir_id(db, abs_path);
  if (! dir_id.has_value()) {
    spdlog::error("Trying to delete path that doesn't exists in the database: {}", path);
    return false;
  }

  SQLite::Statement del_tracks_metadata_stmt = {db, R"--(
    DELETE FROM t_tracks_metadata
    WHERE track_id in (
      SELECT track_id FROM t_tracks_metadata
      JOIN t_tracks ON t_tracks.id = t_tracks_metadata.track_id
      JOIN t_music_dirs ON t_music_dirs.id = t_tracks.parent_dir_id
      WHERE t_music_dirs.id = ?)
  )--"};
  del_tracks_metadata_stmt.bind(1, uint32_t(*dir_id));

  SQLite::Statement del_tracks_stmt = {db, R"--(
    DELETE FROM t_tracks
    WHERE t_tracks.id IN (
      SELECT t_tracks.id FROM t_tracks
      JOIN t_music_dirs ON t_music_dirs.id = t_tracks.parent_dir_id
      WHERE t_music_dirs.id = ?)
  )--"};
  del_tracks_stmt.bind(1, uint32_t(*dir_id));

  SQLite::Statement stmt{db, "DELETE FROM t_music_dirs WHERE id = ?"};
  stmt.bind(1, uint32_t(*dir_id));

  del_tracks_metadata_stmt.exec();
  del_tracks_stmt.exec();
  stmt.exec();
  return true;
}

optional<MDirId> scan_directory(SQLite::Database &db, const string &path) {
  const string abs_path{fs::canonical(path)};
  if (not fs::exists(abs_path) or not fs::is_directory(abs_path)) {
    spdlog::error("Path doesn't exists or is not a directory: {}", path);
    return nullopt;
  }
  const optional<MDirId> id = insert_music_dir(db, abs_path);
  int i                  = 1;
  for (const auto &dir_entry : fs::recursive_directory_iterator(abs_path)) {
    if (dir_entry.is_regular_file() && Utils::is_supported_file_type(dir_entry.path())) {
      insert_track(db, dir_entry.path(), id);
      spdlog::info("{} - INSERTED: {}", i, dir_entry.path().c_str());
      ++i;
    }
  }
  return id;
}

void build_music_library(SQLite::Database &db) {
  const auto mdirs = get_all_music_dirs(db);
  for (const auto &mdir : mdirs)
    scan_directory(db, mdir.path);
}

/******************************************************************************/
/************************** --| Static Functions |-- **************************/
/******************************************************************************/

static bool Utils::is_supported_file_type(const string &path) {
  static constexpr std::array<std::string_view, 2> exts{".flac", ".mp3"};
  return std::ranges::any_of(exts, [&](const auto &ext) { return path.ends_with(ext); });
}

static optional<TrackMetadata> Utils::load_metadata(
    SQLite::Database &db, const TrackId track_id, const string &file_path
) {
  if (not is_valid_track_id(db, track_id))
    return nullopt;
  TagLib::FileRef fref{file_path.c_str()};
  if (fref.isNull() or fref.tag()->isEmpty())
    return nullopt;

  string title;
  if (not fref.tag()->title().isEmpty())
    title = fref.tag()->title().to8Bit(true);
  else
    title = fs::path{file_path}.filename().replace_extension("");

  optional<size_t> trk_num = fref.tag()->track();
  if (trk_num.value() == 0)
    trk_num = nullopt;

  optional<ArtistId> artist_id = nullopt;
  if (not fref.tag()->artist().isEmpty()) {
    const string artist = fref.tag()->artist().to8Bit(true);
    artist_id           = insert_artist(db, artist);
  }

  optional<AlbumId> album_id = nullopt;
  if (not fref.tag()->album().isEmpty()) {
    const string album = fref.tag()->album().to8Bit(true);
    album_id           = insert_album(db, album, artist_id);
  }
  // Extract album art and store it in a file
  const string album_art_filename = std::format("{}/{}", data_dir, album_id.value());
  spdlog::info("{}", album_art_filename);
  if (not fs::exists(album_art_filename)) {
    optional<TagLib::ByteVector> pic = get_album_art(file_path);
    if (pic.has_value()) {
      std::ofstream album_art_file{album_art_filename};
      for (const auto b : pic.value()) {
        album_art_file << b;
      }
    }
  }
  return TrackMetadata(track_id, title, trk_num, artist_id, album_id);
}

static optional<TagLib::ByteVector> Utils::get_flac_album_art(const string &filename) {
  if (not is_supported_file_type(filename))
    return nullopt;

  TagLib::FLAC::File f{filename.c_str()};
  if (not f.isValid() or f.pictureList().isEmpty())
    return nullopt;
  return f.pictureList().front()->data();
}

static optional<TagLib::ByteVector> Utils::get_mp3_album_art(const string &filename) {
  if (not is_supported_file_type(filename) or not filename.ends_with(".mp3"))
    return nullopt;

  TagLib::MPEG::File f{filename.c_str()};
  if (not f.isValid() or not f.hasID3v2Tag())
    return nullopt;
  auto *tags      = f.ID3v2Tag();
  auto &framelist = tags->frameList("APIC");
  if (framelist.isEmpty())
    return nullopt;
  auto *pic = static_cast<TagLib::ID3v2::AttachedPictureFrame *>(framelist.front());
  return pic->picture();
}

static optional<TagLib::ByteVector> Utils::get_album_art(const string &filename) {
  if (not is_supported_file_type(filename))
    return nullopt;
  else if (filename.ends_with(".flac"))
    return get_flac_album_art(filename);
  else  // .mp3
    return get_mp3_album_art(filename);
}

static optional<TrackId> Utils::insert_metadata(SQLite::Database &db, const TrackMetadata &tm) {
  SQLite::Statement stmt{db, R"--(
      INSERT OR REPLACE INTO t_tracks_metadata (track_id, title, track_num, artist_id, album_id)
      VALUES (?, ?, ?, ?, ?);
  )--"};
  stmt.bind(1, uint32_t(tm.track_id));
  if (tm.title.empty())
    stmt.bind(2);
  else
    stmt.bindNoCopy(2, tm.title);

  if (tm.track_number.has_value())
    stmt.bind(3, uint32_t(*tm.track_number));
  else
    stmt.bind(3);

  if (tm.artist_id.has_value())
    stmt.bind(4, uint32_t(*tm.artist_id));
  else
    stmt.bind(4);

  if (tm.album_id.has_value())
    stmt.bind(5, uint32_t(*tm.album_id));
  else
    stmt.bind(5);

  stmt.exec();

  return tm.track_id;
}

}  // namespace Midx
