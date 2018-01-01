
libvalce_sources = [
  'libvalce.c',
]

libvalce_headers = [
  'libvalce.h',
]

install_headers(libvalce_headers,
  subdir: 'libvalce'
)

version_split = meson.project_version().split('.')
MAJOR_VERSION = version_split[0]
MINOR_VERSION = version_split[1]
MICRO_VERSION = version_split[2]
API_VERSION = MAJOR_VERSION + '.0'

version_conf = configuration_data()
version_conf.set('VERSION', meson.project_version())
version_conf.set('MAJOR_VERSION', MAJOR_VERSION)
version_conf.set('MINOR_VERSION', MINOR_VERSION)
version_conf.set('MICRO_VERSION', MICRO_VERSION)

configure_file(
  input: 'libvalce-version.h.in',
  output: 'libvalce-version.h',
  configuration: version_conf,
  install: true,
  install_dir: join_paths(get_option('includedir'), 'libvalce')
)

libvalce_deps = [
  dependency('gio-2.0', version: '>= 2.50'),
  dependency('libvlc', version: '>= 3.0.0'),
]

libvalce_lib = shared_library('valce-' + API_VERSION,
  libvalce_sources,
  dependencies: libvalce_deps,
  install: true,
  install_dir: get_option('libdir'),
)

gnome = import ('gnome')

libvalce_gir = gnome.generate_gir (libvalce_lib,
  sources : [libvalce_headers, libvalce_sources],
  nsversion : API_VERSION,
  namespace : 'Valce',
  symbol_prefix : 'valce',
 identifier_prefix : 'Valce',
  export_packages : 'Valce',
  includes : ['GObject-2.0','GLib-2.0'],
  header : 'libvalce/libvalce.h',
  install : true,
  install_dir_gir: [join_paths(get_option('datadir'), 'gir-1.0')],
  install_dir_typelib: [join_paths(get_option('libdir'), 'girepository-1.0')],
)

vapi_dir = join_paths(get_option('datadir'), 'vala', 'vapi')

libvalce_vapi = gnome.generate_vapi('libvalce-' + API_VERSION,
  sources: libvalce_gir[0],
  packages: ['gio-2.0',],
  install: true,
  install_dir: vapi_dir,
)

pkg = import('pkgconfig')

pkg.generate(
  description: 'A shared library for making GObject based audio player',
    libraries: libvalce_lib,
         name: 'libvalce',
     filebase: 'libvalce-' + API_VERSION,
      version: meson.project_version(),
      subdirs: 'libvalce',
     requires: 'glib-2.0',
  install_dir: join_paths(get_option('libdir'), 'pkgconfig')
)
