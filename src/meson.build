# Modified for the GdMap fork on 2026-11-19.
sources = [
  'about.c',
  'colors.c',
  'filter.c',
  'gui_main.c',
  'gui_support.c',
  'main.c',
  'preferences.c',
  'tree.c',
  'tree_preferences.c',
  'utils.c',
]

cc = meson.get_compiler('c')
cflags = cc.get_supported_arguments('-Wno-deprecated-declarations', '-Wno-pointer-sign')

deps = [
  dependency('cairo'),
  dependency('gtk+-3.0', version: '>=3.22'),
  cc.find_library('m', required: false),
  dependency('libxml-2.0'),
]

executable('gdmap', sources,
  c_args: cflags,
  dependencies: deps,
  include_directories: [config_dir],
  install: true,
)

cli = executable('gdmap-cli', ['cli.c', 'snapshot.c', 'tree.c'],
  dependencies: dependency('glib-2.0', version: '>=2.38'),
  include_directories: [config_dir],
  install: true,
)

python = find_program('python3')
test('snapshot-roundtrip', python,
  args: [files('test_snapshot.py'), cli, meson.project_source_root()],
  timeout: 30,
)
