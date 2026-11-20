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

## Saved scans without a display

`gdmap-cli scan FOLDER SNAPSHOT` scans once, saves the tree, and prints its records. `gdmap-cli inspect SNAPSHOT` prints the saved records in a new process without accessing the original folder. Keep the snapshot outside the scanned folder. For example:

```sh
./build/local/bin/gdmap-cli scan ./src /tmp/src.gdmap
./build/local/bin/gdmap-cli inspect /tmp/src.gdmap
meson test -C build --print-errorlogs
```

The tab-separated output contains the node index, parent index, kind (`directory`, `file`, or `symlink`), bytes, and original absolute path. The root is index zero and references itself. Paths use C escapes for arbitrary filename bytes, tabs and newlines. Children retain the scanner's descending size order, including its original order for ties. The header records the scan policy. The versioned binary GVariant snapshot stores byte names, node kinds, sizes and parent references in little-endian form; readers reject invalid records and depths greater than 1024.

CLI scans default to reported file sizes and stay on the root filesystem. Use `--allocated-size` to count allocated bytes when positive and smaller than the file size, or `--leave-device` to descend onto other filesystems. Symbolic links count their own size, hard links count once per name, and special files are skipped. A directory's size is the sum of accepted children; inaccessible descendants may be skipped by the scanner. These sizes do not guarantee how much space deletion would free. A root that cannot be scanned causes a nonzero exit status. The CLI neither reads nor writes the user's settings or palette and does not require a display.

## Compact snapshot overview

`gdmap-cli overview SNAPSHOT` lists the largest files, branches and repeated directory names. `gdmap-cli expand SNAPSHOT` lists the root's children; select a branch using its printed node number or its exact original absolute path:

```sh
./build/local/bin/gdmap-cli --limit 5 overview /tmp/src.gdmap
./build/local/bin/gdmap-cli --limit 5 --node 1 expand /tmp/src.gdmap
./build/local/bin/gdmap-cli --path '/original/scan/root/folder with spaces' expand /tmp/src.gdmap
```

Node numbers match `inspect` and stay valid for the same snapshot. Paths are displayed as C-escaped bytes, including UTF-8; copy the node number into `--node` to avoid decoding or shell escaping. `--path` accepts actual filename bytes, rather than the displayed escapes. Both operations use the saved tree even after the source moves and preserve the user's settings.

For a series of expansions, run `gdmap-cli expand SNAPSHOT --batch`. After `ready`, send one node number per line. Each response uses the ordinary expansion format and ends with `end`. An invalid number returns an error for that request. Send `quit` or close stdin to finish. The process retains the snapshot and subtree counts in memory. Node numbers from `hit` work in this session, and any number from an expansion can be passed to `render --node` on the same snapshot.

Each node row gives bytes, regular-file count, symlink count and the largest regular file's percentage of that node's bytes (`none` for zero bytes). The default limit is 10 rows per section and per group's member list. Expansions and group member lists report the number and bytes of omitted children or members. Repeated groups combine directories with the same final path component, count only the outermost occurrence when those directories nest, and sort by total bytes. Different branches and different groups can overlap, so their rows must not be added together; omitted overlapping branches or groups report a count without an additive byte total. All sizes retain the snapshot's measurement policy.

## Snapshot maps and point lookup

`render` builds a PNG without a display and writes a compact `PNG.frame` beside it. The frame records the absolute snapshot filename, its SHA-256, image dimensions, selected node, layout mode, depth, lighting and palette legend. Keep it with the PNG and retain the snapshot at its recorded location. `hit` refuses a changed snapshot. No list of file rectangles is stored in the frame. These commands use the same layout, Cairo painting and half-open point lookup as GTK; CLI defaults use the built-in palette and never load or save the user's profile.

```sh
./build/local/bin/gdmap-cli render /tmp/src.gdmap /tmp/src.png --width 1200 --height 800
./build/local/bin/gdmap-cli hit /tmp/src.png.frame --x 400 --y 250
./build/local/bin/gdmap-cli hit /tmp/src.png.frame --x 200 --y 125 --preview-width 600 --preview-height 400
./build/local/bin/gdmap-cli render /tmp/src.gdmap /tmp/branch.png --node 42 --depth 3 --lighting 0.35
./build/local/bin/gdmap-cli render /tmp/src.gdmap /tmp/branch.png --path '/original/scan/root/branch' --mode standard
```

Coordinates start at the top left. The one-pixel outer border is a miss. Shared boundaries belong to the tile on the right or below. Hit rows contain kind, bytes, C-escaped original path and `node=N`; use that number with `render --node N` or `expand --node N`, including for names containing arbitrary bytes. A second row identifies the parent within the rendered subtree; `--parent` selects it. A miss prints `miss`. PNG dimensions are the original coordinate space; pass both preview dimensions to transform coordinates from a scaled display. Dimensions must be 3–32767 pixels, with at most 100 million pixels total. The default layout is `square`; `standard` alternates split axes using the snapshot's original depths. `--depth N` limits visible depth relative to the selected root; zero is unlimited. Zero-byte entries have no tile in a nonempty directory; an empty root can still be selected inside its blank frame.

The `.frame` file's `[legend]` section lists base colors and extensions, with default and folder colors separately. `--palette /explicit/path/colors.xml` reads an explicit GdMap palette without changing it. The default lighting is `1`, matching GTK's original cushions; `--lighting 0.35` softens shadows and `0` uses flat base colors. Lighting changes colors, never geometry. GTK continues to use its existing preferences and palette.

For repeated clicks on a large snapshot, use one prepared process. `hit --batch` loads the snapshot and calculates geometry once, prints `ready WIDTH HEIGHT`, then accepts one `X Y` or `parent X Y` line at a time. Each answer ends with `end` and is flushed immediately. `quit` or EOF closes it. Preview dimensions and `--parent` can also be supplied as process-wide options. This keeps each subsequent click independent of snapshot loading time and avoids a server.

```sh
printf '400 250\nparent 400 250\n0 0\nquit\n' | ./build/local/bin/gdmap-cli hit /tmp/src.png.frame --batch
```

## License

The combined project and changes specific to this fork are available under the [GNU General Public License version 2.0 only](LICENSES/GPL-2.0.txt). Upstream GTK 3 port contributions are licensed under GPL-2.0-or-later unless otherwise stated.
