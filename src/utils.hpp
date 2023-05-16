#pragma once

#include <optional>
#include <string>

namespace MusicIndexer {

class MusicDir {
 public:
  MusicDir(const std::string& path, const std::optional<int> rowid = std::nullopt)
      : m_rowid{rowid}, m_path{path} {
  }
  const std::optional<int> get_rowid() const {
    return m_rowid;
  };
  void set_rowid(const int new_rowid) {
    m_rowid = new_rowid;
  };
  const std::string& get_path() const {
    return m_path;
  };

 private:
  std::optional<int> m_rowid;
  std::string m_path;
};

class Artist {
 public:
  Artist(const std::string& name, const std::optional<int> rowid = std::nullopt)
      : m_rowid{rowid}, m_name{name} {
  }
  const std::optional<int> get_rowid() const {
    return m_rowid;
  };
  void set_rowid(const int new_rowid) {
    m_rowid = new_rowid;
  };
  const std::string& get_name() const {
    return m_name;
  };

 private:
  std::optional<int> m_rowid;
  std::string m_name;
};

class Album {
 public:
  Album(const std::string& name, const std::optional<int> rowid = std::nullopt,
        const std::optional<int> artist_rowid = std::nullopt)
      : m_rowid{rowid}, m_name{name}, m_artist_rowid{artist_rowid} {
  }
  const std::optional<int> get_rowid() const {
    return m_rowid;
  };
  void set_rowid(const int new_rowid) {
    m_rowid = new_rowid;
  };
  const std::string& get_name() const {
    return m_name;
  };
  const std::optional<int> get_artist_rowid() const {
    return m_artist_rowid;
  };
  void set_artist_rowid(const int id) {
    m_artist_rowid = id;
  };

 private:
  std::optional<int> m_rowid;
  std::string m_name;
  std::optional<int> m_artist_rowid;
};

class Track {
 public:
  Track(const std::string& file_path, const std::optional<int> rowid = std::nullopt,
        const std::optional<int> parent_dir_rowid = std::nullopt)
      : m_rowid{rowid}, m_file_path{file_path}, m_parent_dir_rowid{parent_dir_rowid} {
  }
  const std::optional<int> get_rowid() const {
    return m_rowid;
  };
  void set_rowid(const int new_rowid) {
    m_rowid = new_rowid;
  };
  const std::string& get_file_path() const {
    return m_file_path;
  };
  const std::optional<int> get_parent_dir_rowid() const {
    return m_parent_dir_rowid;
  };
  void set_parent_dir(const int id) {
    m_parent_dir_rowid = id;
  };

 private:
  std::optional<int> m_rowid;
  std::string m_file_path;
  std::optional<int> m_parent_dir_rowid;
};

}  // namespace MusicIndexer