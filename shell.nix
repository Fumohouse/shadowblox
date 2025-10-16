{ pkgs ? import <nixpkgs> { } }:

pkgs.mkShell.override { stdenv = pkgs.clang19Stdenv; } {
  nativeBuildInputs = with pkgs; [
    scons
    clang-tools

    # Formatter
    treefmt
    nixpkgs-fmt
    python312
    python312Packages.black
  ];

  shellHook = ''
    alias sbx-build-dbg="SCONS_CACHE=build/dbg scons config=debug target=editor debug_symbols=yes"
    alias sbx-build-relwithdbg="SCONS_CACHE=build/relwithdbg scons config=relwithdbg target=template_release debug_symbols=yes"
    alias sbx-build-release="SCONS_CACHE=build/release scons config=release target=template_release"
  '';
}
