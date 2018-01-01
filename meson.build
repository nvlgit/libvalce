project('libvalce', 'c',        version: '3.0.1',
  meson_version: '>= 0.40.0',
)

config_h = configuration_data()
configure_file(
  output: 'libvalce-config.h',
  configuration: config_h,
)
add_project_arguments([
  '-I' + meson.build_root(),
], language: 'c')

licenses_subdir = join_paths(get_option('datadir'), 'licenses', meson.project_name())
install_data('LICENSE',
    install_dir: licenses_subdir)

doc_subdir = join_paths(get_option('datadir'), 'doc', meson.project_name())
install_data('README.md',
    install_dir: doc_subdir)


subdir('src')
