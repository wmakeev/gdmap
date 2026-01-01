## Updating translatable strings

- Run `meson compile GdMap-pot && meson compile GdMap-update-po` from a Meson build directory.

## Release checklist

- Update version in `meson.build`.
- Update `ChangeLog`.
- Run `git tag -a "vX.Y.Z" -m "GdMap X.Y.Z"`.
- Push the tag.
- Make sure automated builds succeed.
- Create a release.
