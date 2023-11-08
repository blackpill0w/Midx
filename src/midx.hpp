#pragma once

#include <optional>
#include <string>
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
inline std::string data_dir = std::string(getenv("HOME")) + "/.local/share/music-indexer";

/**
 * Initialise database and tables, this function also enables foreign keys check so it is
 * preferred to call it before any operations are done.
 */
void init_database(SQLite::Database &db);

/**
 *  Get all music directories, artists, albums or tracks in the database.
 */
template<typename T>
std::vector<T> get_all([[maybe_unused]] SQLite::Database &db) {
  constexpr bool valid_type = std::is_convertible<T, MusicDir>() or
                              std::is_convertible<T, Artist>() or std::is_convertible<T, Album>() or
                              std::is_convertible<T, Track>();
  static_assert(valid_type, "Invalid DataType to Midx::get_all()\n");
  return {};
}

/**
  Get data related to an artist, album or track's metadata given its id.
*/
template<typename T>
std::optional<T> get([[maybe_unused]] SQLite::Database &db, [[maybe_unused]] const int id) {
  constexpr bool valid_type = std::is_convertible<T, Artist>() or std::is_convertible<T, Album>() or
                              std::is_convertible<T, TrackMetadata>();
  static_assert(valid_type, "Invalid DataType to Midx::get()\n");
  return {};
}

/**
 * Checks if the id of a music directory, artist, album or track is valid.
 */
template<typename T>
inline bool is_valid_id([[maybe_unused]] SQLite::Database &db, [[maybe_unused]] const int id) {
  constexpr bool valid_type = std::is_convertible<T, MusicDir>() or
                              std::is_convertible<T, Artist>() or std::is_convertible<T, Album>() or
                              std::is_convertible<T, Track>();
  static_assert(valid_type, "------------ ERROR: Invalid DataType to Midx::is_valid_id()\n");
  return false;
}

/**
 * Get the id of a music directory, artist, album or track in the database, std::nullopt
 * is return if it doesn't exist.
 * @note The interpretation of the paramaters depends on the function's instance, refer to
 * the relevant function
 */
template<typename T>
std::optional<int> get_id([[maybe_unused]] SQLite::Database &db,
                          [[maybe_unused]] const std::string &str,
                          [[maybe_unused]] const std::optional<int> dummy = std::nullopt) {
  constexpr bool valid_type = std::is_convertible<T, MusicDir>() or
                              std::is_convertible<T, Artist>() or std::is_convertible<T, Album>() or
                              std::is_convertible<T, Track>();
  static_assert(valid_type, "------------ ERROR: Invalid DataType to Midx::get_id()\n");
  return std::nullopt;
}

/**
 * Insert a music directory, an artist, an album or a track in the database
 * @note The interpretation of the paramaters depends on the function's instance, refer to
 * the relevant function instance's documentation
 */
template<typename T>
std::optional<int> insert([[maybe_unused]] SQLite::Database &db,
                          [[maybe_unused]] const std::string &str,
                          [[maybe_unused]] const std::optional<int> dummy = std::nullopt) {
  constexpr bool valid_type = std::is_convertible<T, MusicDir>() or
                              std::is_convertible<T, Artist>() or std::is_convertible<T, Album>() or
                              std::is_convertible<T, Track>();
  static_assert(valid_type, "------------ ERROR: Invalid data type to Midx::insert()\n");
  return std::nullopt;
}

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
