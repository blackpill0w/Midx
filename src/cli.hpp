#pragma once

#include <iostream>
#include <vector>

#include "./database_operations.hpp"

namespace MusicIndexer::CLI {

struct Command {
  // TODO: better names??
  enum class SubCommand
  {
    Unknown,
    Add,
    List
  };
  enum class OperateOn
  {
    None,
    MusicDir,
    Artist,
    Album,
    Track
  };

  SubCommand cmd = SubCommand::Unknown;
  OperateOn what = OperateOn::None;
};

inline void print_help() {
  std::cout << "Usage: music-indexer <COMMAND> args...\n"
               "Commands: \n"
               "\thelp\n"
               "\tadd <DIRECTORY>...\n"
               "\tlist music-dirs|mdirs|albums|artists|tracks\n";
}

inline Command parse_input(int argc, const char *argv[]) {
  struct Command res {};
  if (argc == 1)
    return res;
  // Convert all to std::string
  std::vector<std::string> args{};
  args.reserve(size_t(argc) - 1);
  for (int i = 1; i < argc; ++i)
    args.emplace_back(std::string(argv[i]));

  if (args[0] == "add" and args.size() >= 2) {
    res.cmd = Command::SubCommand::Add;
  }
  else if (args[0] == "list" and args.size() >= 2) {
    res.cmd = Command::SubCommand::List;
    if (args[1] == "music-dirs" or args[1] == "mdirs")
      res.what = Command::OperateOn::MusicDir;
    else if (args[1] == "albums")
      res.what = Command::OperateOn::Album;
    else if (args[1] == "artists")
      res.what = Command::OperateOn::Artist;
    else if (args[1] == "tracks")
      res.what = Command::OperateOn::Track;
  }

  return res;
}
}  // namespace MusicIndexer::CLI
