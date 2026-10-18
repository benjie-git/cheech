#!/bin/sh
#
# Build a portable Windows directory for the cheech binaries.
#
# This is invoked by "make bundle" on MinGW/MSYS2 hosts (see the top-level
# Makefile.am).  It copies the already-built executables into a Cheech/
# directory along with the cheechweb client files (on WIN32,
# AjaxServer::conv_filename serves cheechweb/ next to the exe) and GLib
# schemas, then walks each executable's PE import table with objdump and
# copies any non-system DLL it finds, so the result also runs on machines
# without MSYS2 installed.  A --static build normally depends only on
# standard Windows DLLs, in which case no DLLs are copied.
#
# Requirements:
#   * built in the MSYS2 UCRT64 shell
#   * objdump in PATH (installed with the toolchain)

set -eu

srcdir="."
builddir="."
name="Cheech"
version=""

# Default the version to the AC_INIT version in configure.ac, the single
# source of truth for the cheech version.
if [ -n "$srcdir" ] && [ -f "$srcdir/configure.ac" ]; then
	version=$(tr '\n' ' ' < "$srcdir/configure.ac" \
		| sed -n 's/.*AC_INIT(\[[^]]*\], *\[\([0-9][0-9.]*\)\].*/\1/p')
fi

while [ $# -gt 0 ]; do
	case "$1" in
		--srcdir)   srcdir="$2"; shift 2 ;;
		--builddir) builddir="$2"; shift 2 ;;
		--name)     name="$2"; shift 2 ;;
		--version)  version="$2"; shift 2 ;;
		*) echo "make-bundle.sh: unknown option: $1" >&2; exit 2 ;;
	esac
done

srcdir=$(cd "$srcdir" && pwd)
builddir=$(cd "$builddir" && pwd)

bundle="$builddir/$name"

echo "make-bundle.sh: assembling $bundle"
rm -rf "$bundle"
mkdir -p "$bundle"

# Executables; the GUI is built only when the GUI is enabled.
progs=""
for prog in cheech cheechd cheechbot cheechwebd; do
	bin="$builddir/src/$prog.exe"
	if [ -f "$bin" ]; then
		cp "$bin" "$bundle/"
		progs="$progs $prog"
	fi
done
if [ -z "$progs" ]; then
	echo "make-bundle.sh: no executables found in $builddir/src; run 'make' first" >&2
	exit 1
fi

# Web client files, served by the binaries' built-in AjaxServer, which
# looks in cheechweb/ next to the executable on WIN32.
webdir="$bundle/cheechweb"
if [ -d "$srcdir/web" ]; then
	(
		cd "$srcdir/web"
		find . -type f ! -name 'Makefile*' ! -name '*.bak' -print
	) | while IFS= read -r f; do
		f="${f#./}"
		mkdir -p "$webdir/$(dirname "$f")"
		cp "$srcdir/web/$f" "$webdir/$f"
	done
fi

# GLib schemas; copy only GTK's own (org.gtk.*), not every MSYS2 app's.
# Copied under both share/ and etc/ so GLib finds them however it resolves
# the install root relative to the executable.
prefix=$(pkg-config --variable=prefix gtkmm-4.0 2>/dev/null || true)
if [ -n "$prefix" ] && [ -d "$prefix/share/glib-2.0/schemas" ]; then
	for sub in share etc; do
		mkdir -p "$bundle/$sub/glib-2.0/schemas"
		for f in "$prefix/share/glib-2.0/schemas"/org.gtk.*.gschema.xml; do
			[ -e "$f" ] || continue
			cp -L "$f" "$bundle/$sub/glib-2.0/schemas/"
		done
		if command -v glib-compile-schemas >/dev/null 2>&1; then
			glib-compile-schemas "$bundle/$sub/glib-2.0/schemas" 2>/dev/null || true
		fi
	done
fi

# DLLs that ship with Windows and are never copied.  vulkan-1.dll is
# also skipped: libepoxy loads it opportunistically and GTK renders
# fine without it, and the copy found in the toolchain PATH may be a
# GPU-driver version not present on the target machine.
system_dlls="\
advapi32.dll bcrypt.dll cfgmgr32.dll comctl32.dll comdlg32.dll \
crypt32.dll dbghelp.dll d3d11.dll dbeng.dll dhcpcsvc.dll dnsapi.dll \
dwmapi.dll dwrite.dll dxgi.dll gdi32.dll gdiplus.dll imm32.dll \
iphlpapi.dll kernel32.dll msimg32.dll msvcrt.dll netapi32.dll \
normaliz.dll ntdll.dll ole32.dll oleaut32.dll opengl32.dll powrprof.dll \
psapi.dll secur32.dll setupapi.dll shcore.dll shell32.dll shlwapi.dll \
synch.dll ucrtbase.dll user32.dll userenv.dll uxtheme.dll version.dll \
vulkan-1.dll winmm.dll winspool.drv wintrust.dll ws2_32.dll wtsapi32.dll"

is_system_dll() {
	case "$1" in
		api-ms-*|ext-ms-*) return 0 ;;
	esac
	for d in $system_dlls; do
		[ "$d" = "$1" ] && return 0
	done
	return 1
}

# Walk the PE import table; copy anything that is neither a system DLL
# nor under the Windows directory, and recurse into what was copied.
copied=""
scan_pe() {
	f="$1"
	for dll in $(objdump -p "$f" 2>/dev/null | sed -n 's/.*DLL Name: *//p'); do
		dll=$(echo "$dll" | tr 'A-Z' 'a-z')
		is_system_dll "$dll" && continue
		case " $copied " in
			*" $dll "*)	continue ;;
		esac
		src=$(command -v "$dll" 2>/dev/null || true)
		case "$src" in
			""|/c/Windows/*|/mnt/c/Windows/*)	continue ;;
		esac
		copied="$copied $dll"
		cp "$src" "$bundle/"
		echo "make-bundle.sh: bundled DLL $dll"
		scan_pe "$bundle/$dll"
	done
}
for exe in "$bundle"/*.exe; do
	scan_pe "$exe"
done

echo "make-bundle.sh: done: $bundle ($progs)"
