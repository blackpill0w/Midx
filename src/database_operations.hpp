#pragma once

#include <SQLiteCpp/SQLiteCpp.h>

#include <optional>
#include <string>
#include <vector>

#include "./utils.hpp"

namespace MusicIndexer::DatabaseOperations {

/**
 * Initialise database and tables, this function also enables foreign keys check so it is
 * preferred to call it before any operations are done.
 */
void init_database(SQLite::Database& db);

/**
 *  Get all music directories, artists, albums or tracks in the database.
 */

template<typename T>
std::vector<T> get(SQLite::Database& db) {
  constexpr bool valid_type =
      std::is_convertible<T, MusicDir>() or std::is_convertible<T, Artist>() or
      std::is_convertible<T, Album>() or std::is_convertible<T, Track>();
  static_assert(valid_type, "Invalid DataType to get()\n");
  return {};
}

/**
 * Checks if the id of a music directory, artist, album or track is valid.
 */
template<typename T>
inline bool is_valid_id(SQLite::Database& db, const int id) {
  constexpr bool valid_type =
      std::is_convertible<T, MusicDir>() or std::is_convertible<T, Artist>() or
      std::is_convertible<T, Album>() or std::is_convertible<T, Track>();
  static_assert(valid_type, "------------ ERROR: Invalid DataType to is_valid_id()\n");
  return false;
}

/**
 * Get the id of a music directory, artist, album or track in the database, std::nullopt
 * is return if it doesn't exist.
 * @note The interpretation of the paramaters depends on the function's instance, refer to
 * the relevant function
 */
template<typename T>
std::optional<int> get_id(
    SQLite::Database& db, const std::string& str,
    [[maybe_unused]] const std::optional<int> dummy = std::nullopt) {
  constexpr bool valid_type =
      std::is_convertible<T, MusicDir>() or std::is_convertible<T, Artist>() or
      std::is_convertible<T, Album>() or std::is_convertible<T, Track>();
  static_assert(valid_type, "------------ ERROR: Invalid DataType to get()\n");
  return std::nullopt;
}

/**
 * Insert a music directory, an artist, an album or a track in the database
 * @note The interpretation of the paramaters depends on the function's instance, refer to
 * the relevant function instance's documentation
 */
template<typename T>
std::optional<int> insert(
    SQLite::Database& db, const std::string& str,
    [[maybe_unused]] const std::optional<int> dummy = std::nullopt) {
  constexpr bool valid_type =
      std::is_convertible<T, MusicDir>() or std::is_convertible<T, Artist>() or
      std::is_convertible<T, Album>() or std::is_convertible<T, Track>();
  static_assert(valid_type, "------------ ERROR: Invalid DataType to insert()\n");
  return std::nullopt;
}

/**
 * Recursively scan a directory given its relative or absolute path.
 */
std::optional<int> scan_directory(SQLite::Database& db, const std::string& path,
                                  const std::optional<int> parent_dir_id = std::nullopt);

}  // namespace MusicIndexer::DatabaseOperations
