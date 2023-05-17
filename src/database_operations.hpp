#pragma once

#include <sqlite_modern_cpp.h>

#include <optional>
#include <string>
#include <vector>

#include "./utils.hpp"

namespace MusicIndexer::DatabaseOperations {

class MusicData {
 public:
  MusicData(const std::vector<MusicDir>& music_dirs, const std::vector<Artist>& artists,
            const std::vector<Album>& albums, const std::vector<Track>& tracks)
      : m_music_dirs{music_dirs}, m_artists{artists}, m_albums{albums}, m_tracks{tracks} {
  }
  std::vector<MusicDir>& get_music_dirs() {
    return m_music_dirs;
  }
  std::vector<Artist>& get_artists() {
    return m_artists;
  }
  std::vector<Album>& get_albums() {
    return m_albums;
  }
  std::vector<Track>& get_tracks() {
    return m_tracks;
  }

 private:
  std::vector<MusicDir> m_music_dirs{};
  std::vector<Artist> m_artists{};
  std::vector<Album> m_albums{};
  std::vector<Track> m_tracks{};
};

/**
   Initialise database and tables, this function also enables foreign keys check so it is
   preferred to call it before any operations are done.
*/
void init_database(sqlite::database& db);

/**
   Get all music directories, artists, albums or tracks in the database.
*/

template<typename T>
std::vector<T> get(sqlite::database& db) {
  constexpr bool valid_type =
      std::is_convertible<T, MusicDir>() or std::is_convertible<T, Artist>() or
      std::is_convertible<T, Album>() or std::is_convertible<T, Track>();
  static_assert(valid_type, "Invalid DataType to get()\n");
  // Compiler warnings
  return {};
}

/**
  Get rowid of a music directory, artist, album or track in the database, std::nullopt is
  return if it doesn't exist.

  @note The interpretation of the paramaters depends on the function's instance, refer to
  the relevant function
*/
template<typename T>
std::optional<int> get_rowid(
    sqlite::database& db, const std::string& str,
    [[maybe_unused]] const std::optional<int> dummy = std::nullopt) {
  constexpr bool valid_type =
      std::is_convertible<T, MusicDir>() or std::is_convertible<T, Artist>() or
      std::is_convertible<T, Album>() or std::is_convertible<T, Track>();
  static_assert(valid_type, "Invalid DataType to get()\n");
  // Compiler warnings
  return std::nullopt;
}

/**
  Insert a music directory, an artist, an album or a track in the database.

  @note The interpretation of the paramaters depends on the function's instance, refer to
  the relevant function
*/
template<typename T>
std::optional<int> insert(
    sqlite::database& db, const std::string& str,
    [[maybe_unused]] const std::optional<int> dummy = std::nullopt) {
  constexpr bool valid_type =
      std::is_convertible<T, MusicDir>() or std::is_convertible<T, Artist>() or
      std::is_convertible<T, Album>() or std::is_convertible<T, Track>();
  static_assert(valid_type, "Invalid DataType to get()\n");
  // Compiler warnings
  return std::nullopt;
}

}  // namespace MusicIndexer::DatabaseOperations