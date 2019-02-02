install_man('gdmap.1')

i18n.merge_file(
  input: 'gdmap.desktop.in',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'applications'),
  output: 'gdmap.desktop',
  po_dir: '../po',
  type: 'desktop',
)

install_data('gdmap_icon.png', install_dir: join_paths(get_option('datadir'), 'gdmap', 'pixmaps'))
install_data('gdmap_icon.png', install_dir: join_paths(get_option('datadir'), 'pixmaps'))
