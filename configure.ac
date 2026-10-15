dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([cheech],
        [1.1],
        [https://github.com/benjie-git/cheech],
        [cheech],
        [https://github.com/benjie-git/cheech])
AC_CONFIG_SRCDIR([src/cheech.cc])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])

AM_INIT_AUTOMAKE([foreign subdir-objects -Wall])
AM_MAINTAINER_MODE([enable])

AC_PREFIX_DEFAULT([/usr/local])

AC_CANONICAL_HOST

dnl ---------------------------------------------------------------
dnl Programs
dnl ---------------------------------------------------------------
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S

dnl ---------------------------------------------------------------
dnl Language standard: the sources require C++17 (gtkmm4 / sigc++3).
dnl ---------------------------------------------------------------
AC_LANG_PUSH([C++])
AC_MSG_CHECKING([whether $CXX accepts -std=c++17])
save_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS -std=c++17"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
  [AC_MSG_RESULT([yes])],
  [CXXFLAGS="$save_CXXFLAGS"
   AC_MSG_RESULT([no])
   AC_MSG_ERROR([a C++17 capable compiler is required])])
AC_LANG_POP([C++])

dnl ---------------------------------------------------------------
dnl Link-time optimization.  The AI search calls a large number of
dnl tiny board accessors that live in other translation units; LTO
dnl lets the compiler inline them, which roughly doubles single-core
dnl search speed.  Enabled by default when the toolchain supports it.
dnl ---------------------------------------------------------------
AC_ARG_ENABLE([lto],
  [AS_HELP_STRING([--disable-lto],
                  [disable link-time optimization (LTO) [default=enabled]])],
  [], [enable_lto=yes])

if test "x$enable_lto" = "xyes"; then
  AC_MSG_CHECKING([whether the toolchain supports link-time optimization])
  save_CFLAGS="$CFLAGS"
  save_CXXFLAGS="$CXXFLAGS"
  save_LDFLAGS="$LDFLAGS"
  CFLAGS="$CFLAGS -flto"
  CXXFLAGS="$CXXFLAGS -flto"
  LDFLAGS="$LDFLAGS -flto"
  AC_LANG_PUSH([C++])
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
    [AC_MSG_RESULT([yes])],
    [CFLAGS="$save_CFLAGS"
     CXXFLAGS="$save_CXXFLAGS"
     LDFLAGS="$save_LDFLAGS"
     AC_MSG_RESULT([no])
     enable_lto=no])
  AC_LANG_POP([C++])
fi

dnl ---------------------------------------------------------------
dnl Native Language Support (optional).
dnl There is currently no translation catalogue shipped, so this is
dnl disabled by default (matching the historical build).
dnl ---------------------------------------------------------------
AC_ARG_ENABLE([nls],
  [AS_HELP_STRING([--enable-nls],
                  [enable Native Language Support (gettext) [default=no]])],
  [], [enable_nls=no])

GETTEXT_PACKAGE=cheech
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"],
                   [Package name for gettext])

if test "x$enable_nls" = "xyes"; then
  AC_DEFINE([ENABLE_NLS], [1], [Define if NLS is enabled])
  AC_CHECK_HEADERS([libintl.h], [],
    [AC_MSG_ERROR([--enable-nls requires the gettext development files])])
  AC_CHECK_FUNCS([bind_textdomain_codeset])
  AC_CHECK_LIB([intl], [bindtextdomain])
fi

dnl ---------------------------------------------------------------
dnl PACKAGE_* install directories
dnl ---------------------------------------------------------------
if test "x$prefix" = "xNONE"; then
  packageprefix="$ac_default_prefix"
else
  packageprefix="$prefix"
fi

packagedatadir=share
packagedocdir=doc/$PACKAGE
packagepixmapsdir=${packagedatadir}/pixmaps
packagehelpdir=${packagedatadir}/help
packagemenudir=${packagedatadir}

AC_DEFINE_UNQUOTED([PACKAGE_LOCALE_DIR],
  ["${packageprefix}/${DATADIRNAME}/locale"], [Package local directory])
AC_DEFINE_UNQUOTED([PACKAGE_DATA_DIR],
  ["${packageprefix}/${packagedatadir}"], [Package data directory])
AC_DEFINE_UNQUOTED([PACKAGE_DOC_DIR],
  ["${packageprefix}/${packagedocdir}"], [Package documentation directory])
AC_DEFINE_UNQUOTED([PACKAGE_PIXMAPS_DIR],
  ["${packageprefix}/${packagepixmapsdir}"], [Package pixmaps directory])
AC_DEFINE_UNQUOTED([PACKAGE_HELP_DIR],
  ["${packageprefix}/${packagehelpdir}"], [Package help directory])
AC_DEFINE_UNQUOTED([PACKAGE_MENU_DIR],
  ["${packageprefix}/${packagemenudir}"], [Package menu directory])
AC_DEFINE_UNQUOTED([PACKAGE_SOURCE_DIR],
  ["`cd $srcdir && pwd`"], [Package source directory])

NO_PREFIX_PACKAGE_DATA_DIR="${packagedatadir}"
AC_SUBST([NO_PREFIX_PACKAGE_DATA_DIR])
NO_PREFIX_PACKAGE_DOC_DIR="${packagedocdir}"
AC_SUBST([NO_PREFIX_PACKAGE_DOC_DIR])
NO_PREFIX_PACKAGE_PIXMAPS_DIR="${packagepixmapsdir}"
AC_SUBST([NO_PREFIX_PACKAGE_PIXMAPS_DIR])
NO_PREFIX_PACKAGE_HELP_DIR="${packagehelpdir}"
AC_SUBST([NO_PREFIX_PACKAGE_HELP_DIR])
NO_PREFIX_PACKAGE_MENU_DIR="${packagemenudir}"
AC_SUBST([NO_PREFIX_PACKAGE_MENU_DIR])

dnl ---------------------------------------------------------------
dnl Mac OS X
dnl ---------------------------------------------------------------
dnl Optional support for running from a Mac OS X application bundle.
dnl When enabled the GUI looks up its resources (pixmaps) inside the
dnl .app via CoreFoundation instead of PACKAGE_PIXMAPS_DIR, and the
dnl CoreFoundation framework is linked in.
AC_ARG_ENABLE([macos-app],
  [AS_HELP_STRING([--enable-macos-app],
                  [build the GUI to run from a Mac OS X .app bundle [default=yes on macOS]])],
  [], [AS_CASE([$host],
        [*-apple-darwin*], [enable_macos_app=yes],
        [enable_macos_app=no])])

MACOS_APP_CFLAGS=
MACOS_APP_LIBS=
AS_IF([test "x$enable_macos_app" = "xyes"], [
  AC_DEFINE([MACOS_APP], [1],
            [Define when the GUI is built to run from a Mac OS X .app bundle])
  MACOS_APP_LIBS="-framework CoreFoundation"
])
AC_SUBST([MACOS_APP_CFLAGS])
AC_SUBST([MACOS_APP_LIBS])
AM_CONDITIONAL([ENABLE_MACOS_APP], [test "x$enable_macos_app" = "xyes"])

dnl Look up pkg-config before anything that might need it (PKG_CHECK_MODULES
dnl relies on it, and its probe must not end up inside a conditional).
PKG_PROG_PKG_CONFIG

dnl ---------------------------------------------------------------
dnl Windows (MinGW-w64 toolchain, e.g. the MSYS2 UCRT64 shell).
dnl The unix-style build (autoreconf/configure/make) is used as-is.
dnl The non-system libraries (glib/glibmm/gtkmm and everything they
dnl pull in, plus libgcc/libstdc++) are linked statically so the
dnl binaries depend only on Windows itself; Windows system import
dnl libraries (ws2_32, shlwapi) are used as usual.
dnl ---------------------------------------------------------------
AC_ARG_ENABLE([static-libs],
  [AS_HELP_STRING([--disable-static-libs],
                  [on Windows, link glib/glibmm/gtkmm etc. dynamically instead of statically [default=static]])],
  [], [AS_CASE([$host],
         [*-mingw*|*-msys*], [enable_static_libs=yes],
         [enable_static_libs=no])])

is_win32_host=no
AS_CASE([$host], [*-mingw*|*-msys*], [is_win32_host=yes])
AM_CONDITIONAL([WIN32_HOST], [test "x$is_win32_host" = "xyes"])

WIN32_CFLAGS=
WIN32_LIBS=
WIN32_GUI_LDFLAGS=
AS_IF([test "x$is_win32_host" = "xyes"], [
  WIN32_CFLAGS="-DWIN32"
  dnl With LTO on MinGW/PE, the vague-linkage destructors, vtables and
  dnl thunks emitted into .gnu.linkonce sections by the LTO partitions
  dnl are not deduplicated by ld, causing "multiple definition" errors
  dnl (GCC PR 94156).  A single partition emits each symbol only once,
  dnl which keeps LTO's whole-program inlining.
  AS_IF([test "x$enable_lto" = "xyes"], [
    WIN32_CFLAGS="$WIN32_CFLAGS -flto-partition=none"
    dnl the option must also reach the final LTO link
    LDFLAGS="$LDFLAGS -flto-partition=none"
    dnl With LTO the vague-linkage symbols reach ld through the plugin
    dnl as strong definitions instead of weak ones, so ld fails to
    dnl dedupe the .gnu.linkonce copies and errors out with "multiple
    dnl definition" (binutils PR 26198, GCC PR 94156).  The copies are
    dnl identical, so keeping the first is safe.
    LDFLAGS="$LDFLAGS -Wl,--allow-multiple-definition"
  ])
  WIN32_LIBS="-lws2_32"
  dnl keep the GUI binary from opening a console window
  WIN32_GUI_LDFLAGS="-mwindows"
  dnl never pick up run-time dependencies on libgcc_s_seh-1.dll /
  dnl libstdc++-6.dll
  LDFLAGS="$LDFLAGS -static-libgcc -static-libstdc++"
  AS_IF([test "x$enable_static_libs" = "xyes"], [
    dnl Make pkg-config add Libs.private/Cflags.private (pcre2, ffi,
    dnl gettext, iconv, -DGLIB_STATIC_COMPILATION, ...) so the
    dnl dependencies link statically.
    AS_IF([test -n "$PKG_CONFIG"], [PKG_CONFIG="$PKG_CONFIG --static"])
  ])
])
AC_SUBST([WIN32_CFLAGS])
AC_SUBST([WIN32_LIBS])
AC_SUBST([WIN32_GUI_LDFLAGS])

dnl ---------------------------------------------------------------
dnl Checks for libraries
dnl ---------------------------------------------------------------
AC_CHECK_LIB([socket], [main])
AC_CHECK_LIB([nsl], [main])
AC_CHECK_LIB([resolv], [main])

dnl ---------------------------------------------------------------
dnl GUI (gtkmm) is optional: the server/bot binaries only need
dnl glibmm.  --disable-gui skips gtkmm entirely (headless server
dnl builds); the default (auto) enables the GUI when gtkmm-4.0 is
dnl available.
dnl ---------------------------------------------------------------
AC_ARG_ENABLE([gui],
  [AS_HELP_STRING([--disable-gui],
                  [don't build the GTK GUI; server/bot binaries only [default=auto]])],
  [], [enable_gui=auto])

PKG_CHECK_MODULES([GLIBMM], [glibmm-2.68])
PKG_CHECK_MODULES([GIOMM], [giomm-2.68])

AS_IF([test "x$enable_gui" = "xauto"], [
  PKG_CHECK_MODULES([GTKMM], [gtkmm-4.0],
    [enable_gui=yes],
    [enable_gui=no
     AC_MSG_WARN([gtkmm-4.0 not found; building server/bot binaries only])])
], [test "x$enable_gui" = "xyes"], [
  PKG_CHECK_MODULES([GTKMM], [gtkmm-4.0])
])

AM_CONDITIONAL([ENABLE_GUI], [test "x$enable_gui" = "xyes"])

AC_MSG_CHECKING([whether to build the GTK GUI])
AC_MSG_RESULT([$enable_gui])

AS_IF([test "x$enable_gui" = "xyes"], [
  PKG_CHECK_VAR([GLIB_COMPILE_RESOURCES], [gio-2.0], [glib_compile_resources])
])

dnl GLib / GThread
GLIB_REQ=2.6.0

PKG_CHECK_MODULES([GLIB], [glib-2.0 >= $GLIB_REQ], , [
  AC_MSG_ERROR([
    GLib-2.x development files not found (or version too old):

    Cannot find glib-2.0 >= $GLIB_REQ

    Please check the following:

     * Do you have the glib2 _development_ package installed?
       (usually called glib2-devel, libglib2.0-dev or similar)

     * Did you install glib from source? If yes, you might need to
       point pkg-config to the location of the glib-2.0.pc file by
       doing something like
          export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
       and then running ./configure again.
  ])
])

PKG_CHECK_MODULES([GTHREAD], [gthread-2.0 >= $GLIB_REQ], ,
  [AC_MSG_ERROR([The impossible happened: gthread-2.0 not found])])

dnl make double sure GLib was compiled with GThread support
AC_MSG_CHECKING([if GLib was compiled with support for threads])
SAVED_CFLAGS="$CFLAGS"
CFLAGS="$GTHREAD_CFLAGS"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <glib.h>]], [[
  #ifndef G_THREADS_ENABLED
  #error "cheech requires a GLib with support for threads"
  #endif
]])], [
  AC_MSG_RESULT([yes])
], [
  AC_MSG_RESULT([no])
  AC_MSG_ERROR([cheech requires a GLib with support for threads enabled!])
])
CFLAGS="$SAVED_CFLAGS"

dnl ---------------------------------------------------------------
dnl Compiler characteristics
dnl ---------------------------------------------------------------
if test "x$GCC" = "xyes"; then
  CFLAGS="$CFLAGS -Wall -Wstrict-prototypes \
    -Wmissing-prototypes -Wmissing-declarations"
  if test "x$enable_debug" = "xyes"; then
    CFLAGS="$CFLAGS -Werror"
  fi
  if test "x$enable_ansi" = "xyes"; then
    CFLAGS="$CFLAGS -ansi -pedantic"
  fi
fi

dnl ---------------------------------------------------------------
dnl Optional: libcheck unit test framework
dnl ---------------------------------------------------------------
PKG_CHECK_MODULES([CHECK], [check >= 0.9.4],
  [AM_CONDITIONAL([HAVE_CHECK], [true])],
  [AM_CONDITIONAL([HAVE_CHECK], [false])])

dnl ---------------------------------------------------------------
dnl Optional: valgrind
dnl ---------------------------------------------------------------
AC_PATH_PROG([VALGRIND_PATH], [valgrind], [no])
AC_MSG_CHECKING([whether valgrind.h is available])
AM_CONDITIONAL([HAVE_VALGRIND], [test ! "x$VALGRIND_PATH" = "xno"])
PKG_CHECK_MODULES([VALGRIND], [valgrind > 2.1], [
  AC_MSG_RESULT([yes])
  AC_DEFINE([HAVE_VALGRIND], [1], [Whether valgrind headers are available])
], [
  AC_MSG_RESULT([no])
])

dnl ---------------------------------------------------------------
dnl Output
dnl ---------------------------------------------------------------
dnl On MinGW, -static-libstdc++ makes the gcc driver link the static
dnl libstdc++.a; the -lstdc++ that some static .pc files inject (e.g.
dnl harfbuzz) would additionally pull in libstdc++.dll.a, and ld
dnl rejects the overlapping definitions.  Drop the pkg-config copy.
AS_IF([test "x$is_win32_host" = "xyes"], [
  GTKMM_LIBS=`AS_ECHO_N([" $GTKMM_LIBS "]) | sed 's/ -lstdc+\\+ / /g;s/^ *//;s/ *$//'`
  GLIBMM_LIBS=`AS_ECHO_N([" $GLIBMM_LIBS "]) | sed 's/ -lstdc+\\+ / /g;s/^ *//;s/ *$//'`
  GIOMM_LIBS=`AS_ECHO_N([" $GIOMM_LIBS "]) | sed 's/ -lstdc+\\+ / /g;s/^ *//;s/ *$//'`
  GTHREAD_LIBS=`AS_ECHO_N([" $GTHREAD_LIBS "]) | sed 's/ -lstdc+\\+ / /g;s/^ *//;s/ *$//'`
  GLIB_LIBS=`AS_ECHO_N([" $GLIB_LIBS "]) | sed 's/ -lstdc+\\+ / /g;s/^ *//;s/ *$//'`
])

dnl Deduplicate the library lists.  The pkg-config modules overlap heavily
dnl (gtkmm brings in glibmm/giomm/sigc/glib, and glibmm, giomm, gthread and
dnl glib all repeat -lglib-2.0 and friends), and macOS ld warns
dnl "ignoring duplicate libraries" on every link line they touch.
cheech_core_libs=
for cheech_lib in $GLIBMM_LIBS $GIOMM_LIBS $GTHREAD_LIBS $GLIB_LIBS; do
  case " $cheech_core_libs " in
  *" $cheech_lib "*) ;;
  *) cheech_core_libs="$cheech_core_libs $cheech_lib" ;;
  esac
done
CHEECH_CORE_LIBS=`AS_ECHO_N(["$cheech_core_libs"]) | sed 's/^ *//;s/ *$//'`
AS_IF([test "x$enable_gui" = "xyes"], [
  cheech_gui_libs=
  for cheech_lib in $GTKMM_LIBS $CHEECH_CORE_LIBS; do
    case " $cheech_gui_libs " in
    *" $cheech_lib "*) ;;
    *) cheech_gui_libs="$cheech_gui_libs $cheech_lib" ;;
    esac
  done
  CHEECH_GUI_LIBS=`AS_ECHO_N(["$cheech_gui_libs"]) | sed 's/^ *//;s/ *$//'`
], [
  CHEECH_GUI_LIBS="$CHEECH_CORE_LIBS"
])
AC_SUBST([CHEECH_CORE_LIBS])
AC_SUBST([CHEECH_GUI_LIBS])
AC_CONFIG_FILES([
  Makefile
  src/Makefile
  pixmaps/Makefile
  ajax/Makefile
  ajax/www/Makefile
  ajax/www/com.benjie.cheech/Makefile
  ajax/www/com.benjie.cheech/images/Makefile
])
AC_OUTPUT
