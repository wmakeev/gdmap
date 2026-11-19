# GdMap fork

This is the GdMap GTK 3 fork maintained at `wmakeev/gdmap`. The starting point is upstream `v1.4.0`. Use `origin` for fork development and `upstream` for updates from the GTK 3 port.

## Treemap and navigation

- Keep the treemap tightly packed. Reserve its area for the tiles and show file details on hover. Use the context menu for actions on a tile.
- Use the existing GTK actions and scan path for new entry points. **Open** scans the folder selected in the chooser. **Scan Root (/)** scans the filesystem root from the File menu or the toolbar.

## Build and verify

- Follow the local build steps in [README.md](README.md). For an existing build directory, rebuild and install into its configured prefix with `meson install -C build`.
- Verify UI changes through the affected controls in the running application. A command-line scan exercises a different path from the chooser. For chooser changes, select a child of the displayed folder and confirm that its path becomes the map root. For scan progress changes, check that cancellation works on a slow scan.

## Documentation

- Record user-visible fork changes under **Unreleased** in [CHANGELOG.fork.md](CHANGELOG.fork.md). [ChangeLog](ChangeLog) records upstream releases.
- Keep each Markdown paragraph or list item on one source line.
