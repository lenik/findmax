i18n = import('i18n')

# zfr: i18n-derive-begin
_po_src = meson.current_source_dir()
_po_build = meson.current_build_dir()
_sh = find_program('sh')
_po_dep_list = run_command(
  _sh, '-c', 'cd "$1" && ls -1 *.po LINGUAS 2>/dev/null || true',
  _po_src, check: false,
).stdout().strip().split('\n')
_po_dep_files = []
foreach _p : _po_dep_list
  if _p != ''
    _po_dep_files += [_p]
  endif
endforeach

_zfr_i18n = find_program('zfr', native: true, required: false)
if _zfr_i18n.found()
  custom_target(
    'i18n-derive',
    command: [
      _sh, '-c',
      'cd "$1" && shift && exec "$@"',
      '_',
      meson.project_source_root(),
      _zfr_i18n, 'i18n', '-b',
      '--po-dir', _po_build,
      '--stamp', _po_build / 'i18n-derive.stamp',
      '--compile-mo',
      '--domain', meson.project_name(),
    ],
    output: 'i18n-derive.stamp',
    depend_files: _po_dep_files,
    build_by_default: true,
    console: true,
  )
  meson.add_install_script(
    _sh, '-c',
    'po="$0"; domain="$1"; loc="$2"; base="${MESON_INSTALL_DESTDIR_PREFIX:-${DESTDIR:-}${MESON_INSTALL_PREFIX:-}}"; for f in "$po"/*.po; do [ -e "$f" ] || continue; lang=${f##*/}; lang=${lang%.po}; mo="$po/$lang/LC_MESSAGES/$domain.mo"; [ -f "$mo" ] || continue; dest="$base/$loc/$lang/LC_MESSAGES/$domain.mo"; mkdir -p "$(dirname "$dest")"; install -m644 "$mo" "$dest"; done',
    _po_build,
    meson.project_name(),
    get_option('localedir'),
  )
endif
# zfr: i18n-derive-end

i18n.gettext(
    'findmax',
    data_dirs: _po_src,
    install: true,
)
