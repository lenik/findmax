project('findmax', 'c',
  version: '1.0.0',
  license: 'GPL-3.0-or-later',
  default_options: [
    'warning_level=2',
    'c_std=c99',
    'optimization=2',
  ],
  authors: ['Lenik <findmax@bodz.net>']
)

# Compiler flags
add_project_arguments('-D_GNU_SOURCE', '-fPIC', language: 'c')

# Dependencies
cc = meson.get_compiler('c')

# Configuration
conf = configuration_data()
conf.set_quoted('VERSION', meson.project_version())

# Install directories
bindir = get_option('bindir')
libdir = get_option('libdir')
includedir = get_option('includedir')
mandir = get_option('mandir')
datadir = get_option('datadir')

# Sources
main_sources = [
  'main.c',
  'file_ops.c',
  'format.c',
]

lib_sources = [
  'file_ops.c',
  'format.c',
]

# Headers
headers = [
  'findmax.h',
]

# Build shared library
libfindmax = shared_library(
  'findmax',
  lib_sources,
  install: true,
  install_dir: libdir,
  soversion: '1',
  version: '1.0.0'
)

# Build executable
findmax = executable(
  'findmax',
  main_sources,
  install: true,
  install_dir: bindir,
  link_with: libfindmax
)

# Install headers
install_headers(headers, install_dir: includedir)

# Install man page
install_man('findmax.1')

# Install bash completion
bash_completion_dir = join_paths(datadir, 'bash-completion', 'completions')
install_data(
  'findmax-completion.bash',
  install_dir: bash_completion_dir,
  rename: 'findmax'
)

# Tests
test('basic_tests', findmax, args: ['-t', '.'])

# pkg-config file
pkgconf = configuration_data()
pkgconf.set('prefix', get_option('prefix'))
pkgconf.set('libdir', libdir)
pkgconf.set('includedir', includedir)
pkgconf.set('VERSION', meson.project_version())

configure_file(
  input: 'findmax.pc.in',
  output: 'findmax.pc',
  configuration: pkgconf,
  install: true,
  install_dir: join_paths(libdir, 'pkgconfig')
)

# Summary
summary({
  'prefix': get_option('prefix'),
  'bindir': bindir,
  'libdir': libdir,
  'includedir': includedir,
  'mandir': mandir,
}, section: 'Directories')

summary({
  'version': meson.project_version(),
  'license': 'GPL-3.0-or-later',
  'author': 'Lenik <findmax@bodz.net>',
}, section: 'Project')
