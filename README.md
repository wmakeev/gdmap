# GdMap

GdMap visualizes disk usage as a rectangular treemap. Tile area represents relative size and color distinguishes file types.

## About this fork

This fork was created to make GdMap useful for everyday disk cleanup. It keeps the shaded, tightly packed treemap and adds file manager and clipboard actions directly to its tiles. Names and sizes are shown on hover so the map area stays available for the visualization.

The fork started on 2026-11-19 from [GdMap 1.4.0](https://gitlab.com/sjohannes/gdmap/-/tags/v1.4.0), the [GTK 3 port](https://gitlab.com/sjohannes/gdmap) of the [original GdMap](https://gdmap.sourceforge.net/).

- Right-click a tile to copy its full path or open its folder in the file manager. For a file tile, this opens the containing folder.
- Use **File → Scan Root (/)** or the disk button on the toolbar to scan the filesystem root directly.
- The folder chooser scans the selected folder. During a scan, GdMap updates progress and accepts cancellation.

Fork changes are recorded in [CHANGELOG.fork.md](CHANGELOG.fork.md). The upstream release history is in [ChangeLog](ChangeLog).

## Build and run on Linux

The build requires a C compiler, Meson, Ninja, gettext, and development files for GTK 3.22 or newer, Cairo, and libxml2.

```sh
meson setup build --prefix="$PWD/build/local"
meson install -C build
./build/local/bin/gdmap
```

Use `./build/local/bin/gdmap --folder /` to scan the root directory at startup.

## License

The combined project and changes specific to this fork are available under the [GNU General Public License version 2.0 only](LICENSES/GPL-2.0.txt). Upstream GTK 3 port contributions are licensed under GPL-2.0-or-later unless otherwise stated.
