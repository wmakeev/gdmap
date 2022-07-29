project('GdMap', 'c',
  license: 'GPL-2.0-only',
  version: '1.2.0',
)

i18n = import('i18n')
gettext_package = meson.project_name()
subdir('po')

subdir('data')

config = configuration_data()
config.set10('ENABLE_NLS', true)
config.set_quoted('GDMAP_LOCALE_DIR', get_option('localedir'))
config.set_quoted('GETTEXT_PACKAGE', gettext_package)
config.set10('HAVE_SYS_TIME_H', true)
config.set_quoted('PACKAGE_DATA_DIR', join_paths(get_option('datadir'), 'gdmap'))
config.set_quoted('PACKAGE_NAME', meson.project_name())
config.set_quoted('PACKAGE_VERSION', meson.project_version())
config.set_quoted('VERSION', meson.project_version())
configure_file(configuration: config, output: 'config.h')

add_project_arguments('-DHAVE_CONFIG_H', language: 'c')
config_dir = include_directories('.')

subdir('src')
