# shadowblox

> **Note**
>
> This repository is mirrored from
> [Forgejo](https://git.seki.pw/Fumohouse/shadowblox) to
> [GitHub](https://github.com/Fumohouse/shadowblox). Issues and pull
> requests are accepted from both sites, but pushes only occur from Forgejo.

Shadow `DataModel` and script runner emulating Roblox APIs and behaviors. Hooks
into a frontend that provides rendering, input, physics, networking, etc., such
as [Godot](https://godotengine.org/) or another game engine. Like
[React](https://react.dev/), but for games.

This project is not affiliated with Roblox Corporation.

## Documentation

Developer documentation is generated using
[Doxygen](https://www.doxygen.nl/index.html). Currently, only especially
important details documented. More details may be added in the future.

## License

See `COPYRIGHT.txt`.

## Attributions

- The Roblox Luau API is maintained by Roblox Corporation.
- Rojo team: [Documentation of the `.rbxm` file
  format](https://dom.rojo.space/binary.html). See the credits in the Rojo
  documentation for original sources.
- Pseudoreality: [Documentation of identities and
  capabilities](https://github.com/Pseudoreality/Roblox-Identities).

## See also

- This project supersedes
  [godot-luau-script](https://git.seki.pw/Fumohouse/godot-luau-script), a
  GDExtension adding support for Luau (but not Roblox APIs) to Godot 4.x.
  Various components of shadowblox are based off of godot-luau-script.
