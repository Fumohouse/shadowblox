# -*- mode: python -*-

import os

env_base = Environment(tools=["default"], CC="clang", CXX="clang++", PLATFORM="")

# Find toolchain on PATH
env_base.PrependENVPath("PATH", os.getenv("PATH"))

# clang terminal colors
env_base["ENV"]["TERM"] = os.environ.get("TERM")

opts = Variables([], ARGUMENTS)

opts.Add(
    EnumVariable(
        key="toolchain",
        help="C++ toolchain",
        default="clang",
        allowed_values=("gcc", "clang", "msvc", "msvc-clang"),
    )
)

opts.Add(
    EnumVariable(
        key="config",
        help="Release configuration",
        default="debug",
        allowed_values=("debug", "relwithdbg", "release"),
    )
)

opts.Update(env_base)

if env_base["toolchain"] == "gcc":
    env_base["CC"] = "gcc"
    env_base["CXX"] = "g++"
    env_base["LINK"] = "g++"
    env_base["RANLIB"] = "ranlib"
elif env_base["toolchain"] == "clang":
    env_base["CC"] = "clang"
    env_base["CXX"] = "clang++"
    env_base["LINK"] = "clang++"
    env_base["RANLIB"] = "ranlib"
elif env_base["toolchain"] == "msvc":
    env_base.Tool("msvc")
    env_base.Tool("mslib")
    env_base.Tool("mslink")
elif env_base["toolchain"] == "msvc-clang":
    env_base.Tool("msvc")
    env_base.Tool("mslib")
    env_base.Tool("mslink")
    env_base["CC"] = "clang-cl"
    env_base["CXX"] = "clang-cl"

if env_base["toolchain"] in ["msvc", "msvc-clang"]:
    env_base.Append(CCFLAGS=["/std:c++20", "/EHsc"])
    if env_base["config"] == "debug":
        env_base.Append(CCFLAGS=["/O2", "/Zi", "/FS"])
        env_base.Append(LINKFLAGS=["/DEBUG:FULL"])
    elif env_base["config"] == "relwithdbg":
        env_base.Append(CCFLAGS=["/O3", "/Zi", "/FS"])
        env_base.Append(LINKFLAGS=["/DEBUG:FULL"])
    elif env_base["config"] == "release":
        env_base.Append(CCFLAGS=["/O3"])
else:
    env_base.Append(CCFLAGS=["-std=c++20"])
    if env_base["config"] == "debug":
        env_base.Append(CCFLAGS=["-O2", "-g"])
    elif env_base["config"] == "relwithdbg":
        env_base.Append(CCFLAGS=["-O3", "-g"])
    elif env_base["config"] == "release":
        env_base.Append(CCFLAGS=["-O3"])

env_base.Tool("compilation_db")
cdb = env_base.CompilationDatabase()
Default(cdb)

env_extern_base = env_base.Clone()

if env_base["toolchain"] in ["msvc", "msvc-clang"]:
    env_base.Append(CCFLAGS=["/W4"])
else:
    env_base.Append(CCFLAGS=["-Wall"])

Export("env_base")
Export("env_extern_base")

extern_includes = ["#extern/include/"]
Export("extern_includes")

luau_lib, luau_includes = SConscript("extern/SCSub_Luau")
Export(["luau_lib", "luau_includes"])

shadowblox_lib, shadowblox_includes = SConscript("shadowblox/SCSub")
Export(["shadowblox_lib", "shadowblox_includes"])

SConscript("shadowblox_tests/SCSub")

# TODO: GDExtension should invoke this SCons script, not the other way around
# SConscript("shadowblox_godot/SCSub")
