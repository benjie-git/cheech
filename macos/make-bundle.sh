#!/bin/sh
#
# Build a relocatable Mac OS X .app bundle for the cheech GUI.
#
# This is invoked by "make bundle" (see the top-level Makefile.am).  It
# copies the already-built GUI into a Cheech.app skeleton, copies the
# program's data files into Contents/Resources (where the CoreFoundation
# code in game_images.cc looks for them, i.e. configure --enable-macos-app),
# vendors the non-system dynamic libraries with dylibbundler and ad-hoc
# code signs the result.
#
# Requirements:
#   * configured with --enable-macos-app
#   * dylibbundler in PATH  (brew install dylibbundler)

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

if [ -z "$srcdir" ] || [ -z "$builddir" ]; then
	echo "make-bundle.sh: --srcdir and --builddir are required" >&2
	exit 2
fi

srcdir=$(cd "$srcdir" && pwd)
builddir=$(cd "$builddir" && pwd)

bin="$builddir/src/cheech"
if [ ! -x "$bin" ]; then
	echo "make-bundle.sh: $bin not found; run 'make' first" >&2
	exit 1
fi

if ! grep -q '^#define MACOS_APP 1' "$builddir/config.h" 2>/dev/null; then
	echo "make-bundle.sh: this build was not configured for a Mac app bundle." >&2
	echo "              Re-run configure with --enable-macos-app and rebuild." >&2
	exit 1
fi

if ! command -v dylibbundler >/dev/null 2>&1; then
	echo "make-bundle.sh: dylibbundler not found; install it with 'brew install dylibbundler'" >&2
	exit 1
fi

app="$builddir/$name.app"
macos="$app/Contents/MacOS"
res="$app/Contents/Resources"
frameworks="$app/Contents/Frameworks"

echo "make-bundle.sh: assembling $app"
rm -rf "$app"
mkdir -p "$macos" "$res" "$frameworks"

# The real executable and a tiny launcher that sets up the GTK runtime
# environment before exec'ing it.  CFBundleExecutable points at the
# launcher; CoreFoundation still resolves the enclosing .app as the main
# bundle when the real binary runs.
cp "$bin" "$macos/cheech-bin"
chmod 755 "$macos/cheech-bin"

# (The About dialog's cheech.png is compiled into the binary as a GResource;
# pixmaps on disk are only used by the Linux make-install desktop icon.)

# Application icon, generated from the artwork that ships with the old
# Xcode project, if it is available.
icon_file=""
icon_src="$srcdir/cheech/cheech/Assets.xcassets/AppIcon.appiconset/cheechApp128.png"
if [ -f "$icon_src" ] && command -v sips >/dev/null 2>&1; then
	if sips -s format icns "$icon_src" --out "$res/$name.icns" >/dev/null 2>&1; then
		icon_file="$name.icns"
	fi
fi

# GTK/GLib runtime data.  GTK 4 embeds its theme and icons in libgtk's
# GResource, so no icon themes or theme files are needed; PNG loading is
# built into gdk-pixbuf itself, so no pixbuf loader modules are bundled
# either (the app loads no image files of its own).  GSettings schemas,
# however, must exist on disk:
# copy only GTK's own (org.gtk.*) schemas, not every Homebrew app's.
brew_prefix=""
if command -v brew >/dev/null 2>&1; then
	brew_prefix=$(brew --prefix 2>/dev/null || true)
fi
if [ -z "$brew_prefix" ] && command -v pkg-config >/dev/null 2>&1; then
	# Derive the Homebrew root from a pkg-config prefix (e.g.
	# /opt/homebrew/opt/gtk4 or /opt/homebrew/Cellar/glib/2.84.0
	# -> /opt/homebrew).
	for m in gtkmm-4.0 gtk4 glib-2.0 gdk-pixbuf-2.0; do
		p=$(pkg-config --variable=prefix "$m" 2>/dev/null || true)
		[ -n "$p" ] || continue
		case "$p" in
			*/Cellar/*) brew_prefix=${p%%/Cellar/*} ;;
			*)          brew_prefix=$p ;;
		esac
		break
	done
fi

if [ -n "$brew_prefix" ] && [ -d "$brew_prefix/share" ]; then
	schemas_src="$brew_prefix/share/glib-2.0/schemas"
	if [ -d "$schemas_src" ]; then
		mkdir -p "$res/share/glib-2.0/schemas"
		for f in "$schemas_src"/org.gtk.*.gschema.xml; do
			[ -e "$f" ] || continue
			# -L: Homebrew populates its share dirs with relative
			# symlinks into the Cellar; copy the real files so the
			# bundle contains no links pointing outside itself
			# (Gatekeeper rejects those, and they would dangle anyway).
			cp -L "$f" "$res/share/glib-2.0/schemas/"
		done
		if command -v glib-compile-schemas >/dev/null 2>&1; then
			glib-compile-schemas "$res/share/glib-2.0/schemas" 2>/dev/null || true
		fi
	fi
fi

# Web client files, served by the app's built-in AjaxServer.  The launcher
# below exports CHEECH_RESOURCES, and AjaxServer::conv_filename looks in
# share/cheech/cheechweb under it (same relative layout as make install).
webdir="$res/share/cheech/cheechweb"
if [ -d "$srcdir/ajax/www/com.benjie.cheech" ]; then
	mkdir -p "$webdir/images"
	for f in "$srcdir"/ajax/www/com.benjie.cheech/*; do
		case "$f" in
			*/Makefile*|*.bak)	continue ;;
		esac
		if [ -f "$f" ]; then
			cp "$f" "$webdir/"
		fi
	done
	for f in "$srcdir"/ajax/www/com.benjie.cheech/images/*.png; do
		[ -e "$f" ] || continue
		cp "$f" "$webdir/images/"
	done
fi

# Launcher script.
cat > "$macos/cheech" <<EOF
#!/bin/sh
# Generated by cheech's make bundle.  Sets up the GTK runtime environment
# so the bundled libraries and data are used, then runs the game.
bundle="\$(cd "\$(dirname "\$0")/../.." && pwd)"
resources="\$bundle/Contents/Resources"

CHEECH_RESOURCES="\$resources"
GTK_DATA_PREFIX="\$resources"
GTK_EXE_PREFIX="\$resources"
GTK_PATH="\$resources"
XDG_DATA_DIRS="\$resources/share${brew_prefix:+:$brew_prefix/share}"
export CHEECH_RESOURCES GTK_DATA_PREFIX GTK_EXE_PREFIX GTK_PATH XDG_DATA_DIRS

if [ -d "\$resources/share/glib-2.0/schemas" ]; then
	GSETTINGS_SCHEMA_DIR="\$resources/share/glib-2.0/schemas"
	export GSETTINGS_SCHEMA_DIR
fi

exec "\$bundle/Contents/MacOS/cheech-bin" "\$@"
EOF
chmod 755 "$macos/cheech"

# Info.plist.
cat > "$app/Contents/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleDevelopmentRegion</key>
	<string>en</string>
	<key>CFBundleExecutable</key>
	<string>cheech</string>
	<key>CFBundleIdentifier</key>
	<string>com.octopilot.cheech</string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>CFBundleName</key>
	<string>$name</string>
	<key>CFBundleDisplayName</key>
	<string>$name</string>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>CFBundleShortVersionString</key>
	<string>$version</string>
	<key>CFBundleVersion</key>
	<string>$version</string>
	<key>CFBundleIconFile</key>
	<string>$icon_file</string>
	<key>LSMinimumSystemVersion</key>
	<string>11.0</string>
	<key>NSHighResolutionCapable</key>
	<true/>
	<key>NSPrincipalClass</key>
	<string>NSApplication</string>
</dict>
</plist>
EOF

# Bundle the dynamic libraries.  dylibbundler walks the dependency graph,
# copies each non-system library into Contents/Frameworks and rewrites the
# install names to point at @executable_path/../Frameworks.
echo "make-bundle.sh: bundling libraries with dylibbundler"
dylibbundler -b -cd \
	-x "$macos/cheech-bin" \
	-d "$frameworks" \
	-p "@executable_path/../Frameworks" </dev/null

# Ad-hoc code signature so macOS is happy to launch the bundle locally.
if command -v codesign >/dev/null 2>&1; then
	codesign --force --deep --sign - "$app" >/dev/null 2>&1 || \
		echo "make-bundle.sh: warning: ad-hoc codesign failed (the app still runs locally)" >&2
fi

echo "make-bundle.sh: done: $app"
