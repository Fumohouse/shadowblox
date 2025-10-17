{ pkgs ? import <nixpkgs> { } }:

pkgs.mkShell.override { stdenv = pkgs.clang19Stdenv; } {
  nativeBuildInputs = with pkgs; [
    scons
    clang-tools

    valgrind

    # Formatter
    treefmt
    nixpkgs-fmt
    python312
    python312Packages.black
  ];

  shellHook = ''
    # Build
    alias dbg="SCONS_CACHE=build/dbg scons config=debug target=editor debug_symbols=yes"
    alias relwithdbg="SCONS_CACHE=build/relwithdbg scons config=relwithdbg target=template_release debug_symbols=yes"
    alias release="SCONS_CACHE=build/release scons config=release target=template_release"

    # Test
    alias t="valgrind --leak-check=full shadowblox_tests/shadowblox_tests"
    # Build + Test
    alias bt="dbg && t"

    # Format + Lint
    alias f="treefmt && scripts/licenser"
    alias l="git ls-files | grep -E '\.(cpp|hpp)$' | xargs clang-tidy"
  '';
}
