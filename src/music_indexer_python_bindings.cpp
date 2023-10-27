#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include "./music_indexer.hpp"

namespace mi = MusicIndexer;

PYBIND11_MODULE(music_indexer, handle) {
  handle.doc() =
      "Library to index music files and their metadata, with the intention to be used as a backend "
      "for a music player.";

  // Global variables
  handle.attr("SQLite_OPEN_READWRITE") = &SQLite::OPEN_READWRITE;
  handle.attr("SQLite_OPEN_READONLY")  = &SQLite::OPEN_READONLY;
  handle.attr("SQLite_CREATE")         = &SQLite::OPEN_CREATE;
  // TODO: find some way to define docstring for global variable
  handle.attr("DATA_DIR")              = &mi::data_dir;

  py::class_<SQLite::Database>(handle, "SQLiteDB").def(py::init<char *, int>());

  py::class_<mi::MusicDir>(handle, "MusicDir")
      .def(py::init<const std::string &, const int>())
      .def_readonly("path", &mi::MusicDir::path)
      .def_readonly("id", &mi::MusicDir::id)
      .def("__str__", [&](mi::MusicDir &mdir) {
        return "MusicDir(id=" + std::to_string(mdir.id) + ", path='" + mdir.path + "')";
      });

  py::class_<mi::Artist>(handle, "Artist")
      .def(py::init<const int, const std::string &>())
      .def_readonly("id", &mi::Artist::id)
      .def_readonly("name", &mi::Artist::name)
      .def("__str__", [&](mi::Artist &a) {
        return "Artist(id=" + std::to_string(a.id) + ", name='" + a.name + "')";
      });

  py::class_<mi::Album>(handle, "Album")
      .def(py::init<const std::string &, const int, const std::optional<int>>(), py::arg("name_"),
           py::arg("id_"), py::arg("artist_id_") = py::none())
      .def_readonly("id", &mi::Album::id)
      .def_readonly("name", &mi::Album::name)
      .def_readonly("artist_id", &mi::Album::artist_id)
      .def("__str__", [&](mi::Album &a) {
        return "Album(id=" + std::to_string(a.id) + ", name=" + a.name +
               ", artist_id=" + (a.artist_id ? std::to_string(*a.artist_id) : "None") + ")";
      });

  py::class_<mi::TrackMetadata>(
      handle, "TrackMetadata",
      "Represents a track's metadata, it's supposed to be a read only data structure.")
      .def(py::init<const int, const std::string &, const std::optional<int>,
                    const std::optional<int>, const std::optional<int>>(),
           py::arg("track_id_"), py::arg("title_"), py::arg("track_number_") = py::none(),
           py::arg("artist_id_") = py::none(), py::arg("album_id_") = py::none())
      .def_readonly("track_id", &mi::TrackMetadata::track_id)
      .def_readonly("title", &mi::TrackMetadata::title)
      .def_readonly("track_number", &mi::TrackMetadata::track_number)
      .def_readonly("artist_id", &mi::TrackMetadata::artist_id)
      .def_readonly("album_id", &mi::TrackMetadata::album_id)
      .def("__str__", [&](mi::TrackMetadata &tm) {
        return "TrackMetadata(track_id=" + std::to_string(tm.track_id) +
               ", album_id=" + (tm.album_id ? std::to_string(*tm.album_id) : "None") +
               ", artist_id" + (tm.artist_id ? std::to_string(*tm.artist_id) : "None") +
               ", track_number" + (tm.track_number ? std::to_string(*tm.track_number) : "None") +
               ")";
      });

  py::class_<mi::Track>(handle, "Track")
      .def(py::init<const int, const std::string &, const int>(),
           "The constructor, it does not initialise the `metadata` field, call "
           "`Track::update_metadata()` for that.")
      .def("update_metadata", &mi::Track::update_metadata)
      .def_readonly("id", &mi::Track::id)
      .def_readonly("file_path", &mi::Track::file_path)
      .def_readonly("parent_dir_id", &mi::Track::parent_dir_id)
      .def("__str__", [&](mi::Track &t) {
        return "Track(id=" + std::to_string(t.id) + ", file_path='" + t.file_path +
               "', parent_dir_id=" + std::to_string(t.parent_dir_id) +
               ", track_metadata?=" + (t.get_metadata() ? "True" : "None") + ")";
      });

  handle.def(
      "init_database", &mi::init_database,
      "Initialise database and tables, this function also enables foreign keys checks so it is "
      "preferred to call it before any operations are done.");

  handle.def("get_all_music_dirs", &mi::get_all<mi::MusicDir>);
  handle.def("get_all_artists", &mi::get_all<mi::Artist>);
  handle.def("get_all_albums", &mi::get_all<mi::Album>);
  handle.def("get_all_tracks", &mi::get_all<mi::Track>);

  handle.def("get_artist", &mi::get<mi::Artist>);
  handle.def("get_album", &mi::get<mi::Album>);
  handle.def("get_track_metadata", &mi::get<mi::TrackMetadata>);

  handle.def("is_valid_music_dir_id", &mi::is_valid_id<mi::MusicDir>);
  handle.def("is_valid_artist_id", &mi::is_valid_id<mi::Artist>);
  handle.def("is_valid_album_id", &mi::is_valid_id<mi::Album>);
  handle.def("is_valid_track_id", &mi::is_valid_id<mi::Track>);

  handle.def("get_music_dir_id", &mi::get_id<mi::MusicDir>, py::arg("db"), py::arg("path"),
             py::arg("dummy") = py::none());
  handle.def("get_artist_id", &mi::get_id<mi::Artist>, py::arg("db"), py::arg("name"),
             py::arg("dummy") = py::none());
  handle.def("get_album_id", &mi::get_id<mi::Album>, py::arg("db"), py::arg("name"),
             py::arg("artist_id") = py::none());
  handle.def("get_track_id", &mi::get_id<mi::Track>, py::arg("db"), py::arg("file_path"),
             py::arg("dummy") = py::none());

  handle.def("insert_music_dir", &mi::insert<mi::MusicDir>, py::arg("db"), py::arg("path"),
             py::arg("dummy") = py::none());
  handle.def("insert_artist", &mi::insert<mi::Artist>, py::arg("db"), py::arg("name"),
             py::arg("dummy") = py::none());
  handle.def("insert_album", &mi::insert<mi::Album>, py::arg("db"), py::arg("name"),
             py::arg("artist_id") = py::none());
  handle.def("insert_track", &mi::insert<mi::Track>, py::arg("db"), py::arg("file_path"),
             py::arg("dummy") = py::none());

  handle.def("get_ids_of_tracks_of_music_dir", &mi::get_ids_of_tracks_of_music_dir,
             "Get ids of the tracks that are inside (and bound to) a certain music directory.");

  handle.def("remove_music_dir", &mi::remove_music_dir);

  handle.def("remove_track", &mi::remove_track,
             "Delete a track (and its metadata) from the database.");

  handle.def("scan_directory", &mi::scan_directory,
             "Recursively scan a directory given its relative or absolute path.");

  handle.def(
      "build_music_library", &mi::build_music_library,
      "Scan all directories present in the database and add all the existing tracks, artists...");
}
