# Version is injected by packaging/rpm/Makefile via `zfr version`.
# RPM Version cannot contain '-'; use `zfr version -r` (hyphens → '_').
# srcversion is the unsanitized Meson/git version and names the tarball.
%{!?version:%global version 0.0.0}
%{!?srcversion:%global srcversion %{version}}

Name:           findmax
Version:        %{version}
Release:        1%{?dist}
Summary:        Fast file finding utility optimized for O(1) queries

License:        AGPL-3.0-or-later
URL:            https://github.com/lenik/findmax
Packager:       Lenik <findmax@bodz.net>
Source0:        %{name}-%{srcversion}.tar.xz

BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  pkgconf
BuildRequires:  bash-completion
BuildRequires:  meson
BuildRequires:  ninja-build
BuildRequires:  asciidoctor
BuildRequires:  libbas-c-dev
BuildRequires:  gettext

%description
findmax is a fast file finding utility specifically optimized for O(1) queries
to find files with maximum values for specified criteria. It can find files
based on modification time, access time, creation time, size, or name, with
support for custom output formatting and various filtering options.
.
Key features:
* Fast O(1) performance using min-heap data structures
* Multiple sort criteria (time, size, name)
* Recursive directory traversal with depth limits
* Custom output formatting similar to stat(1)
* File type filtering (files only, directories only)
* Symbolic link dereference support
* Bash completion support

%prep
%setup -q -n %{name}-%{srcversion}

%build
meson setup build \
    --prefix=%{_prefix} \
    --bindir=%{_bindir} \
    --datadir=%{_datadir} \
    --mandir=%{_mandir} \
    --sysconfdir=%{_sysconfdir} \
    --localstatedir=%{_localstatedir} \
    --buildtype=plain
meson compile -C build

%install
meson install -C build --destdir=%{buildroot}

%files
%{_bindir}/findmax
%{_prefix}/lib/*/libfindmax.so*
%{_prefix}/lib/*/pkgconfig/findmax.pc
%{_includedir}/findmax.h
%{_datadir}/bash-completion/completions/findmax
%{_mandir}/man1/findmax.1*
%{_mandir}/*/man1/findmax.1*
%{_datadir}/locale/*/LC_MESSAGES/findmax.mo
%{_datadir}/doc/%{name}/

%changelog
* Thu Aug 20 2026 Lenik <findmax@bodz.net>
- Align spec with debian/control (Meson, AGPL-3.0-or-later).
- Version comes from `zfr version`, the same method meson.build uses.
