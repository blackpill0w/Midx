#pragma once

#include <optional>
#include <vector>
#include <cstdlib>

#include <SQLiteCpp/SQLiteCpp.h>

#include "./utils.hpp"

namespace Midx {

/**
  The directory where the indexer stores album art and other data.
  By default it's <b>"~/.local/share/music-indexer"</b>, it's better to modify it
  to be inside your project's data directory
  (e.g. <b>"~/.local/share/music-player/indexer-data"</b>).

  @note In case it's modified, make sure Midx::data_dir exists
  and the value doesn't end with a '/'.

  @todo modify default value to work on other platforms.

  Album art is stored in the file `Midx::data_dir/id_of_corresponding_album`.
*/
inline std::string data_dir;

/**
 * Initialise database and tables, this function also enables foreign keys check so it is
 * preferred to call it before any operations are done.
 */
void init_database(SQLite::Database &db);

std::vector<MusicDir> get_all_music_dirs(SQLite::Database &db);
std::vector<Artist> get_all_artists(SQLite::Database &db);
std::vector<Album> get_all_albums(SQLite::Database &db);
std::vector<Track> get_all_tracks(SQLite::Database &db);

std::optional<Artist> get_artist(SQLite::Database &db, const int id);
std::optional<Album> get_album(SQLite::Database &db, const int id);
std::optional<Track> get_track(SQLite::Database &db, const int id);
std::optional<TrackMetadata> get_track_metadata(SQLite::Database &db, const int id);

bool is_valid_music_dir_id(SQLite::Database &db, const int id);
bool is_valid_artist_id(SQLite::Database &db, const int id);
bool is_valid_album_id(SQLite::Database &db, const int id);
bool is_valid_track_id(SQLite::Database &db, const int id);

std::optional<int> get_music_dir_id(SQLite::Database &db, const std::string &path);
std::optional<int> get_artist_id(SQLite::Database &db, const std::string &name);
std::optional<int> get_album_id(SQLite::Database &db, const std::string &name,
                                const std::optional<int> artist_id);
std::optional<int> get_track_id(SQLite::Database &db, const std::string &file_path);

std::optional<int> insert_music_dir(SQLite::Database &db, const std::string &path);
std::optional<int> insert_artist(SQLite::Database &db, const std::string &name);
std::optional<int> insert_album(SQLite::Database &db, const std::string &name,
                                const std::optional<int> artist_id);
std::optional<int> insert_track(SQLite::Database &db, const std::string &file_path,
                                const std::optional<int> parent_dir_id);

/**
  Delete a track (and its metadata) from the database.
*/
bool remove_track(SQLite::Database &db, const int track_id);

/**
  Get ids of tracks inside (and bound to) a certain music directory.
*/
std::vector<int> get_ids_of_tracks_of_music_dir(SQLite::Database &db, const int mdir_id);

/**
 * Remove a music directory from the database
 */
bool remove_music_dir(SQLite::Database &db, const std::string &path);

/**
 * Recursively scan a directory given its relative or absolute path.
 */
std::optional<int> scan_directory(SQLite::Database &db, const std::string &path);

/**
 * Scan all directories present in the database and add all the existing tracks,
 * artists...
 */
void build_music_library(SQLite::Database &db);

}  // namespace Midx
