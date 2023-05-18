with import <nixpkgs> {};
  stdenv.mkDerivation {
    name = "music-indexer";
    hardeningDisable = [ "all" ];
    nativeBuildInputs = with pkgs; [
      pkg-config
      cmake
      ninja
      clang-tools
      valgrind
    ];
    buildInputs = with pkgs.buildPackages; [
      sqlite
      taglib
    ];
  }
