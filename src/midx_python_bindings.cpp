#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include "./midx.hpp"

PYBIND11_MODULE(midx, handle) {
  handle.doc() =
      "Library to index music files and their metadata, with the intention to be used as a backend "
      "for a music player.";

  // Global variables
  handle.attr("SQLite_OPEN_READWRITE") = &SQLite::OPEN_READWRITE;
  handle.attr("SQLite_OPEN_READONLY")  = &SQLite::OPEN_READONLY;
  handle.attr("SQLite_CREATE")         = &SQLite::OPEN_CREATE;
  // TODO: find some way to define docstring for global variable
  handle.attr("DATA_DIR") = &Midx::data_dir;

  py::class_<SQLite::Database>(handle, "SQLiteDB").def(py::init<char *, int>());

  py::class_<Midx::MusicDir>(handle, "MusicDir")
      .def(py::init<const std::string &, const int>())
      .def_readonly("path", &Midx::MusicDir::path)
      .def_readonly("id", &Midx::MusicDir::id)
      .def("__str__", [&](Midx::MusicDir &mdir) {
        return "MusicDir(id=" + std::to_string(mdir.id) + ", path='" + mdir.path + "')";
      });

  py::class_<Midx::Artist>(handle, "Artist")
      .def(py::init<const int, const std::string &>())
      .def_readonly("id", &Midx::Artist::id)
      .def_readonly("name", &Midx::Artist::name)
      .def("__str__", [&](Midx::Artist &a) {
        return "Artist(id=" + std::to_string(a.id) + ", name='" + a.name + "')";
      });

  py::class_<Midx::Album>(handle, "Album")
      .def(py::init<const std::string &, const int, const std::optional<int>>(), py::arg("name_"),
           py::arg("id_"), py::arg("artist_id_") = py::none())
      .def_readonly("id", &Midx::Album::id)
      .def_readonly("name", &Midx::Album::name)
      .def_readonly("artist_id", &Midx::Album::artist_id)
      .def("__str__", [&](Midx::Album &a) {
        return "Album(id=" + std::to_string(a.id) + ", name=" + a.name +
               ", artist_id=" + (a.artist_id ? std::to_string(*a.artist_id) : "None") + ")";
      });

  py::class_<Midx::TrackMetadata>(
      handle, "TrackMetadata",
      "Represents a track's metadata, it's supposed to be a read only data structure.")
      .def(py::init<const int, const std::string &, const std::optional<int>,
                    const std::optional<int>, const std::optional<int>>(),
           py::arg("track_id_"), py::arg("title_"), py::arg("track_number_") = py::none(),
           py::arg("artist_id_") = py::none(), py::arg("album_id_") = py::none())
      .def_readonly("track_id", &Midx::TrackMetadata::track_id)
      .def_readonly("title", &Midx::TrackMetadata::title)
      .def_readonly("track_number", &Midx::TrackMetadata::track_number)
      .def_readonly("artist_id", &Midx::TrackMetadata::artist_id)
      .def_readonly("album_id", &Midx::TrackMetadata::album_id)
      .def("__str__", [&](Midx::TrackMetadata &tm) {
        return "TrackMetadata(track_id=" + std::to_string(tm.track_id) +
               ", album_id=" + (tm.album_id ? std::to_string(*tm.album_id) : "None") +
               ", artist_id" + (tm.artist_id ? std::to_string(*tm.artist_id) : "None") +
               ", track_number" + (tm.track_number ? std::to_string(*tm.track_number) : "None") +
               ")";
      });

  py::class_<Midx::Track>(handle, "Track")
      .def(py::init<const int, const std::string &, const int>(),
           "The constructor, it does not initialise the `metadata` field, call "
           "`Track::update_metadata()` for that.")
      .def("update_metadata", &Midx::Track::update_metadata)
      .def_readonly("id", &Midx::Track::id)
      .def_readonly("file_path", &Midx::Track::file_path)
      .def_readonly("parent_dir_id", &Midx::Track::parent_dir_id)
      .def("__str__", [&](Midx::Track &t) {
        return "Track(id=" + std::to_string(t.id) + ", file_path='" + t.file_path +
               "', parent_dir_id=" + std::to_string(t.parent_dir_id) +
               ", track_metadata?=" + (t.get_metadata() ? "True" : "None") + ")";
      });

  handle.def(
      "init_database", &Midx::init_database,
      "Initialise the database and tables, this function also enables foreign keys checks so it is "
      "preferred to call it before any operations are done.");

  handle.def("get_all_music_dirs", &Midx::get_all_music_dirs);
  handle.def("get_all_artists", &Midx::get_all_artists);
  handle.def("get_all_albums", &Midx::get_all_albums);
  handle.def("get_all_tracks", &Midx::get_all_tracks);

  handle.def("get_artist", &Midx::get_artist);
  handle.def("get_album", &Midx::get_album);
  handle.def("get_track_metadata", &Midx::get_track_metadata);

  handle.def("is_valid_music_dir_id", &Midx::is_valid_music_dir_id);
  handle.def("is_valid_artist_id", &Midx::is_valid_artist_id);
  handle.def("is_valid_album_id", &Midx::is_valid_album_id);
  handle.def("is_valid_track_id", &Midx::is_valid_track_id);

  handle.def("get_music_dir_id", &Midx::get_music_dir_id);
  handle.def("get_artist_id", &Midx::get_artist_id);
  handle.def("get_album_id", &Midx::get_album_id);
  handle.def("get_track_id", &Midx::get_track_id);

  handle.def("insert_music_dir", &Midx::insert_music_dir);
  handle.def("insert_artist", &Midx::insert_artist);
  handle.def("insert_album", &Midx::insert_album);
  handle.def("insert_track", &Midx::insert_track);

  handle.def("get_ids_of_tracks_of_music_dir", &Midx::get_ids_of_tracks_of_music_dir,
             "Get ids of the tracks that are inside (and bound to) a certain music directory.");

  handle.def("remove_music_dir", &Midx::remove_music_dir);

  handle.def("remove_track", &Midx::remove_track,
             "Delete a track (and its metadata) from the database.");

  handle.def("scan_directory", &Midx::scan_directory,
             "Recursively scan a directory given its relative or absolute path.");

  handle.def(
      "build_music_library", &Midx::build_music_library,
      "Scan all directories present in the database and add all the existing tracks, artists...");
}
