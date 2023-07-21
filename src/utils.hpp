#pragma once

#include <optional>
#include <string>
#include <SQLiteCpp/SQLiteCpp.h>

namespace MusicIndexer {

class MusicDir {
 public:
  MusicDir(const std::string &path, const int id) : id{id}, path{path} {}

 public:
  const int id;
  const std::string path;
};

class Artist {
 public:
  Artist(const std::string &name, const int id) : id{id}, name{name} {}

 public:
  const int id;
  const std::string name;
};

class Album {
 public:
  Album(const std::string &name, const int id, const std::optional<int> artist_id = std::nullopt)
      : id{id}, name{name}, artist_id{artist_id} {}

 public:
  const int id;
  const std::string name;
  const std::optional<int> artist_id;
};

/**
 * Represents a track's metadata, it's supposed to be a read only data structure.
 * */
class TrackMetadata {
 public:
  TrackMetadata(const TrackMetadata &other) = default;
  explicit TrackMetadata(const int track_id, const std::string &title,
                         const std::optional<int> track_number = std::nullopt,
                         const std::optional<int> artist_id    = std::nullopt,
                         const std::optional<int> album_id     = std::nullopt,
                         const std::optional<int> album_art_id = std::nullopt)
      : track_id{track_id},
        title{title},
        track_number{track_number},
        artist_id{artist_id},
        album_id{album_id},
        album_art_id{album_art_id} {}

 public:
  /**
   * Id of the track the metadata belongs to.
   */
  const int track_id;
  /**
   * Track's title.
   */
  const std::string title;
  /**
   * Track's number.
   */
  const std::optional<int> track_number;
  /**
   * Track's artist's id.
   */
  const std::optional<int> artist_id;
  /**
   * Track's album's id.
   */
  const std::optional<int> album_id;
  /**
   * Album's art id.
   */
  const std::optional<int> album_art_id;
};

class Track {
 public:
  /**
   * The constructor, it does not initialise the `metadata` field, call
   * `Track::update_metadata()` for that.
   */
  Track(const int id, const std::string &file_path, const int parent_dir_id)
      : id{id}, file_path{file_path}, parent_dir_id{parent_dir_id} {}
  const std::optional<TrackMetadata> get_metadata() const { return m_metadata; }
  void update_metadata(const TrackMetadata &metadata) { m_metadata.emplace(metadata); }

 public:
  const int id;
  const std::string file_path;
  const int parent_dir_id;

 private:
  std::optional<TrackMetadata> m_metadata = std::nullopt;
};

}  // namespace MusicIndexer
