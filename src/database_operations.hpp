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
   Get all items in the database.
   Implemented for MusicDir, Artist, Album and Track.

   @param db initialised database with tables already created
*/

template<class T>
std::vector<T> get(sqlite::database& db) {
  constexpr bool valid_type =
      std::is_convertible<T, MusicDir>() or std::is_convertible<T, Artist>() or
      std::is_convertible<T, Album>() or std::is_convertible<T, Track>();
  static_assert(valid_type, "Invalid DataType to get()\n");
}

template<>
std::vector<MusicDir> get<MusicDir>(sqlite::database& db);

template<>
std::vector<Artist> get<Artist>(sqlite::database& db);

template<>
std::vector<Album> get<Album>(sqlite::database& db);

template<>
std::vector<Track> get<Track>(sqlite::database& db);

/**
  Adds a directory to the database, and scans it for tracks.
*/
bool add_music_directory(const std::string& path);

}  // namespace MusicIndexer::DatabaseOperations