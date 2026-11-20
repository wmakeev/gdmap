# GdMap fork changelog

This file records user-visible changes made in the fork. See [ChangeLog](ChangeLog) for upstream releases. Add entries under **Unreleased** as work lands and give them a version and date when publishing a fork release.

## Unreleased

### Added

- Compact `gdmap-cli overview` and `expand` operations show large files, branch totals, repeated directory groups, file counts and largest-file shares, with bounded output and byte-safe navigation by node number or original path.

- Headless `gdmap-cli scan` and `inspect` operations save and reopen a local scan with its original paths, node kinds, sizes, child order and measurement policy, without changing user preferences.

- Treemap context menu for opening a folder in the file manager and copying a tile's full path. File tiles open their containing folders.
- **Scan Root (/)** in the File menu and on the toolbar to start a root scan without the folder chooser.
- Russian translations for the new actions and messages.

### Fixed

- Scan the folder selected in the chooser.
- Use a monotonic clock for progress updates so the window can repaint and the user can cancel during a scan. Disable map navigation during scanning to keep the scan target consistent.

### Changed

- Started from upstream GdMap 1.4.0. The new context menu requires GTK 3.22 or newer.
