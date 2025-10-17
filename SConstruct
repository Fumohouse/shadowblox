# -*- mode: python -*-

env_base = Environment(
    tools=["default", "compilation_db"], CC="clang", CXX="clang++", PLATFORM=""
)

cdb = env_base.CompilationDatabase()
Default(cdb)

opts = Variables([], ARGUMENTS)

opts.Add(
    EnumVariable(
        key="config",
        help="Release configuration",
        default="debug",
        allowed_values=("debug", "relwithdbg", "release"),
    )
)

opts.Update(env_base)

env_base.Append(CCFLAGS=["-std=c++17"])
if env_base["config"] == "debug":
    env_base.Append(CCFLAGS=["-O2", "-g"])
elif env_base["config"] == "relwithdbg":
    env_base.Append(CCFLAGS=["-O3", "-g"])
elif env_base["config"] == "release":
    env_base.Append(CCFLAGS=["-O3"])

Export("env_base")

extern_includes = ["#extern/include/"]
Export("extern_includes")

luau_lib, luau_includes = SConscript("extern/SCSub_Luau")
Export(["luau_lib", "luau_includes"])

shadowblox_lib, shadowblox_includes = SConscript("shadowblox/SCSub")
Export(["shadowblox_lib", "shadowblox_includes"])

SConscript("shadowblox_tests/SCSub")
SConscript("shadowblox_godot/SCSub")
