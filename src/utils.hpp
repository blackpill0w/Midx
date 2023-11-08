#pragma once

#include <optional>
#include <string>
#include <SQLiteCpp/SQLiteCpp.h>

namespace Midx {

class MusicDir {
 public:
  MusicDir(const std::string &path_, const int id_) : id{id_}, path{path_} {}

 public:
  const int id;
  const std::string path;
};

class Artist {
 public:
  Artist(const int id_, const std::string &name_) : id{id_}, name{name_} {}

 public:
  const int id;
  const std::string name;
};

class Album {
 public:
  Album(const std::string &name_, const int id_, const std::optional<int> artist_id_ = std::nullopt)
      : id{id_}, name{name_}, artist_id{artist_id_} {}

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
  explicit TrackMetadata(const int track_id_, const std::string &title_,
                         const std::optional<int> track_number_ = std::nullopt,
                         const std::optional<int> artist_id_    = std::nullopt,
                         const std::optional<int> album_id_     = std::nullopt)
      : track_id{track_id_},
        title{title_},
        track_number{track_number_},
        artist_id{artist_id_},
        album_id{album_id_} {}

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
};

class Track {
 public:
  /**
   * The constructor, it does not initialise the `metadata` field, call
   * `Track::update_metadata()` for that.
   */
  Track(const int id_, const std::string &file_path_, const int parent_dir_id_)
      : id{id_}, file_path{file_path_}, parent_dir_id{parent_dir_id_} {}

  const std::optional<TrackMetadata> &get_metadata() const { return m_metadata; }

  void update_metadata(const TrackMetadata &metadata_) { m_metadata.emplace(metadata_); }

 public:
  const int id;
  const std::string file_path;
  const int parent_dir_id;

 private:
  std::optional<TrackMetadata> m_metadata = std::nullopt;
};

}  // namespace Midx
