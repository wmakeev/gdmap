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
