with import <nixpkgs> {};
  stdenv.mkDerivation {
    name = "music-indexer";
    hardeningDisable = [ "all" ];
    nativeBuildInputs = with pkgs; [
      pkg-config
      gcc
      gnumake
      cmake
      ninja
      clang-tools
      valgrind
    ];
    buildInputs = with pkgs.buildPackages; [
      sqlite
      taglib
      sqlitecpp
      qt6.qtmultimedia
    ];
  }
