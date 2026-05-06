# Development

shadowblox is built using [SCons](https://www.scons.org/). It supports (without
comprehensive testing) the following toolchains:

- GCC (including MinGW)
- LLVM/Clang
- MSVC (Windows only)
- Clang-cl (Windows only)

with no particular restrictions on platform or architecture. C++20 or later is
required. If you experience compilation or runtime issues on any of these
toolchains or any (reasonably common) platform/architecture, please submit an
issue.

Build prerequisites:

- Python 3.14+
- `scons`
- Your toolchain of choice
- Download the Roblox API dump using `python3 -m sbxcg api download` (or
  manually into `sbxcg/data/`)

The main shadowblox library is compiled into a statically-linked library. There
are only two SCons parameters:

- `toolchain`: `gcc`, `clang`, `msvc`, or `msvc-clang`
- `config`: `debug`, `relwithdbg`, `release`

For example, to build with 4 simultaneous jobs for MSVC in debug mode:

```
scons -j4 toolchain=msvc config=debug
```

A test binary is compiled in `shadowblox_tests/shadowblox_tests` (or
`shadowblox_tests\shadowblox_tests.exe` on Windows).

## Development environment

The following additional packages are recommended or required for development.

Testing:

- `valgrind`

Formatting:

- `treefmt`
- `clang-tools` (for `clang-format` and `clang-tidy`)
- `stylua`
- `black` (Python formatter)

Documentation:

- `doxygen`

## `sbxcg`

`sbxcg` is used for code generation and managing the Roblox API dump. It is a
Python module and should be invoked using `python3 -m sbxcg` (this will be
aliased to `sbxcg` in the following sections).

### License headers

Most shadowblox source files have an MPL license header. To add or update the
headers across the entire source tree, use `sbxcg licenser`. If this
tool does not add a header to your source file, then it need not be included.

### Class management

All classes and members present in the Roblox API dump are managed
semi-automatically using `sbxcg`. Auto-generated files are identified using
`// SBXCG ...` comments. In header files, a JSON blob is also included to store
per-member settings. `sbxcg` uses special regions marked by
`/* BEGIN USER CODE <name> */` and `/* END USER CODE <name> */` comments to
preserve modifications between regenerations. This means that all modifications
outside these regions will be lost!

To generate or regenerate a class, run `sbxcg class <name> generate`. To add or
modify members, use `sbxcg class <name> member <name> <arguments>`. The
following arguments apply:

- `--add`: Add the member if it is not already present. Does nothing if the
  member is already present.
- `--remove`: Remove an existing member.

For methods and properties:

- `--alias <target>`: Alias this member to another of the same type (for
  example, `isA` to `IsA`). Metadata is unique; implementations will be shared.

For methods only:

- `--virtual`: Mark a method as virtual.
- `--pure`: Mark a method as pure virtual (i.e., abstract). Implies `--virtual`.
- `--const`: Mark a method as const.
- `--custom`: Use a Lua function for this method rather than a C++ function.

For properties only:

- `--virtual-get`: Mark the getter as virtual.
- `--virtual-set`: Mark the setter as virtual.

Every invocation to the `member` subcommand overrides the entire configuration
for the given member. That is, if you set `--const` and later set `--virtual`
without `--const`, then `--const` will be lost. As such, you must set all flags
you wish to apply at the same time.
