# shadowblox

Shadow `DataModel` and script runner emulating Roblox APIs and behaviors. Hooks
into a frontend that provides rendering, physics, and networking, such
[Godot](https://godotengine.org/) or another game engine. Like
[React](https://react.dev/), but for games.

This project is not affiliated with Roblox Corporation.

## License

See `COPYRIGHT.txt`.

## Attributions

The Roblox Luau API is maintained by Roblox Corporation.

shadowblox is built on reverse engineering work for the `.rbxm` file format
documented by the Rojo team: <https://dom.rojo.space/binary.html>. See the
credits the Rojo documentation for original sources.

## See also

- This project supersedes
  [godot-luau-script](https://git.seki.pw/Fumohouse/godot-luau-script), a
  GDExtension adding support for Luau (but not Roblox APIs) to Godot 4.x.
  Various components of shadowblox are based off of godot-luau-script.
