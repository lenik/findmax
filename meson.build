project(
  'findmax',
  'c',
  version: run_command(
    'sh',
    '-c', '''
    v=$(
        command -v zfr >/dev/null 2>&1 && zfr version 2>/dev/null || true
    )
    if [ -z "$v" ] && [ -f VERSION ]; then
        v=$(head -n1 VERSION | tr -d '\r')
        v=${v#v}
    fi
    if [ -z "$v" ]; then
        v="0.0.0" # FIXED TO 0.0.0, DO NOT MODIFY
    fi
    printf '%s' "$v"
    ''',
    check: false,
  ).stdout().strip(),
  license: 'AGPL-3.0-or-later',
  default_options: [
    'warning_level=2',
    'c_std=c99',
    'optimization=2',
  ],
)

project_author = 'Lenik'
project_email = 'findmax@bodz.net'
project_year = 2026

asciidoctor = find_program('asciidoctor', required: true)
fs = import('fs')
cc = meson.get_compiler('c')

prefix = get_option('prefix')
bindir = prefix / get_option('bindir')
libdir = prefix / get_option('libdir')
includedir = prefix / get_option('includedir')
datadir = prefix / get_option('datadir')
localedir = prefix / get_option('localedir')
mandir = prefix / get_option('mandir')
pkgdocdir = datadir / 'doc' / meson.project_name()

config_h = configuration_data()
config_h.set_quoted('VERSION', meson.project_version())
config_h.set_quoted('PROJECT_VERSION', meson.project_version())
config_h.set_quoted('PROJECT_AUTHOR', project_author)
config_h.set_quoted('PROJECT_EMAIL', project_email)
config_h.set('PROJECT_YEAR', project_year)
config_h.set_quoted('PREFIX', prefix)
config_h.set_quoted('DATADIR', datadir)
config_h.set_quoted('MANDIR', mandir)
config_h.set_quoted('LOCALEDIR', localedir)

configure_file(
  output: 'config.h',
  configuration: config_h,
)

bas_c_dep = dependency('bas-c', required: true)
bas_args = [
  '-DLOGGER_NAME=' + meson.project_name(),
  '-DTEXT_DOMAIN=' + meson.project_name(),
]
c_args_bas = bas_args

add_project_arguments('-D_GNU_SOURCE', '-fPIC', language: 'c')

inc = include_directories('src')

main_sources = [
  'src/main.c',
  'src/file_ops.c',
  'src/format.c',
  'src/heap.c',
]

lib_sources = [
  'src/file_ops.c',
  'src/format.c',
  'src/logger.c',
]

headers = [
  'src/findmax.h',
]

lib_version = meson.project_version().split('-')[0]
lib_soversion = lib_version.split('.')[0]

libfindmax = shared_library(
  'findmax',
  lib_sources,
  include_directories: inc,
  dependencies: [bas_c_dep],
  c_args: c_args_bas,
  install: true,
  install_dir: libdir,
  soversion: lib_soversion,
  version: lib_version,
)

findmax = executable(
  'findmax',
  main_sources,
  include_directories: inc,
  dependencies: [bas_c_dep],
  c_args: c_args_bas,
  install: true,
  install_dir: bindir,
  link_with: libfindmax,
)

install_headers(headers, install_dir: includedir)

bash_completion_dir = datadir / 'bash-completion' / 'completions'
install_data(
  'findmax-completion.bash',
  install_dir: bash_completion_dir,
  rename: 'findmax',
)

test('basic_tests', findmax, args: ['-t', '.'])

pkgconf = configuration_data()
pkgconf.set('prefix', prefix)
pkgconf.set('libdir', get_option('libdir'))
pkgconf.set('includedir', get_option('includedir'))
pkgconf.set('VERSION', meson.project_version())

configure_file(
  input: 'findmax.pc.in',
  output: 'findmax.pc',
  configuration: pkgconf,
  install: true,
  install_dir: libdir / 'pkgconfig',
)

install_data(
  [
    'LICENSE',
    'README.md',
    'README-zh_CN.md',
  ],
  install_dir: pkgdocdir,
)

puffs = [
  'findmax',
]
foreach puff : puffs
  custom_target(
    puff + '-man',
    input: 'docs' / (puff + '.adoc'),
    output: puff + '.1',
    command: [
      asciidoctor,
      '-b', 'manpage',
      '-a', 'project-version=' + meson.project_version(),
      '-a', 'project-year=@0@'.format(project_year),
      '-a', 'project-author=' + project_author,
      '-a', 'project-email=' + project_email,
      '-o', '@OUTPUT@',
      '@INPUT@',
    ],
    build_by_default: true,
    install: true,
    install_dir: mandir / 'man1',
  )
endforeach

run_target(
  'look',
  command: [
    'bash',
    '-euc', '''
        tmpdir=$(mktemp -d)
        DESTDIR=$tmpdir meson install -C "@BUILD_ROOT@"
        tree "$tmpdir"
        rm -fr "$tmpdir"
    ''',
  ],
)

summary({
  'prefix': prefix,
  'bindir': bindir,
  'libdir': libdir,
  'includedir': includedir,
  'mandir': mandir,
}, section: 'Directories')

summary({
  'version': meson.project_version(),
  'license': 'AGPL-3.0-or-later',
  'author': 'Lenik <findmax@bodz.net>',
}, section: 'Project')
