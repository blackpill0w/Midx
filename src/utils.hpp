#pragma once

#include <optional>
#include <string>
#include <SQLiteCpp/SQLiteCpp.h>

namespace MusicIndexer {

class MusicDir {
 public:
  MusicDir(const std::string& path, const int id) : id{id}, path{path} {}

 public:
  const int id;
  const std::string path;
};

class Artist {
 public:
  Artist(const std::string& name, const int id) : id{id}, name{name} {}

 public:
  const int id;
  const std::string name;
};

class Album {
 public:
  Album(const std::string& name, const int id,
        const std::optional<int> artist_id = std::nullopt)
      : id{id}, name{name}, artist_id{artist_id} {}

 public:
  const int id;
  const std::string name;
  const std::optional<int> artist_id;
};

/**
 * Represents a track's metadata, it's supposed to be a read only data structure (for now
 * at least).
 * */
class TrackMetadata {
 public:
  TrackMetadata(const TrackMetadata& other) = default;
  explicit TrackMetadata(const int track_id,
                         const std::optional<std::string> title = std::nullopt,
                         const std::optional<int> track_number  = std::nullopt,
                         const std::optional<int> artist_id     = std::nullopt,
                         const std::optional<int> album_id      = std::nullopt)
      : track_id{track_id},
        m_title{title},
        m_track_number{track_number},
        m_artist_id{artist_id},
        m_album_id{album_id} {}

  const std::optional<std::string> get_title() const { return m_title; }
  std::optional<int> get_track_number() const { return m_track_number; }
  std::optional<int> get_artist_id() const { return m_artist_id; }
  std::optional<int> get_album_id() const { return m_album_id; }

 public:
  const int track_id;

 private:
  std::optional<std::string> m_title;
  std::optional<int> m_track_number = std::nullopt;
  std::optional<int> m_artist_id    = std::nullopt;
  std::optional<int> m_album_id     = std::nullopt;
};

class Track {
 public:
  /**
   * Constructor, it does not initialise the `metadata` field, call
   * `Track::update_metadata()` for that.
   */
  Track(const int id, const std::string& file_path, const int parent_dir_id)
      : id{id}, file_path{file_path}, parent_dir_id{parent_dir_id} {}
  const std::optional<TrackMetadata> get_metadata() const { return m_metadata; }
  void update_metadata(const TrackMetadata& metadata) { m_metadata.emplace(metadata); }

 public:
  const int id;
  const std::string file_path;
  const int parent_dir_id;

 private:
  std::optional<TrackMetadata> m_metadata = std::nullopt;
};

}  // namespace MusicIndexer
