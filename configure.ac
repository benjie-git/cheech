dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([cheech],
        [0.1],
        [http://cheech.sourceforge.net/],
        [cheech],
        [http://cheech.sourceforge.net/])
AC_CONFIG_SRCDIR([src/cheech.cc])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])

AM_INIT_AUTOMAKE([foreign subdir-objects -Wall])
AM_MAINTAINER_MODE([enable])

AC_PREFIX_DEFAULT([/usr/local])

dnl ---------------------------------------------------------------
dnl Programs
dnl ---------------------------------------------------------------
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S

dnl ---------------------------------------------------------------
dnl Language standard: the sources require C++11.
dnl ---------------------------------------------------------------
AC_LANG_PUSH([C++])
AC_MSG_CHECKING([whether $CXX accepts -std=c++11])
save_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS -std=c++11"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
  [AC_MSG_RESULT([yes])],
  [CXXFLAGS="$save_CXXFLAGS"
   AC_MSG_RESULT([no])
   AC_MSG_ERROR([a C++11 capable compiler is required])])
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
AS_CASE([$host],
  [*-apple-darwin*],
    [AC_DEFINE([GNET_APPLE_DARWIN], [1],
               [Define if building on Apple Darwin])])

dnl Optional support for running from a Mac OS X application bundle.
dnl When enabled the GUI looks up its resources (pixmaps) inside the
dnl .app via CoreFoundation instead of PACKAGE_PIXMAPS_DIR, and the
dnl CoreFoundation framework is linked in.
AC_ARG_ENABLE([macos-app],
  [AS_HELP_STRING([--enable-macos-app],
                  [build the GUI to run from a Mac OS X .app bundle [default=no]])],
  [], [enable_macos_app=no])

MACOS_APP_LIBS=
AS_IF([test "x$enable_macos_app" = "xyes"], [
  AC_DEFINE([MACOS_APP], [1],
            [Define when the GUI is built to run from a Mac OS X .app bundle])
  MACOS_APP_LIBS="-framework CoreFoundation"
])
AC_SUBST([MACOS_APP_LIBS])
AM_CONDITIONAL([ENABLE_MACOS_APP], [test "x$enable_macos_app" = "xyes"])

dnl ---------------------------------------------------------------
dnl Checks for libraries
dnl ---------------------------------------------------------------
AC_CHECK_LIB([socket], [main])
AC_CHECK_LIB([nsl], [main])
AC_CHECK_LIB([resolv], [main])

dnl gtkmm (GUIs)
PKG_CHECK_MODULES([GTKMM], [gtkmm-2.4])

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
  #error "GNet requires a GLib with support for threads"
  #endif
]])], [
  AC_MSG_RESULT([yes])
], [
  AC_MSG_RESULT([no])
  AC_MSG_ERROR([GNet requires a GLib with support for threads enabled!])
])
CFLAGS="$SAVED_CFLAGS"

dnl ---------------------------------------------------------------
dnl IPv6 support on unix (runtime detection on win32)
dnl ---------------------------------------------------------------
AS_IF([test "x$gnet_native_win32" != "xyes"], [
  AC_MSG_CHECKING([for IPv6 availability])
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
  ]], [[
    struct sockaddr_in6 s;
    struct in6_addr t = in6addr_any;
    int i = AF_INET6;
    s;
    t.s6_addr[0] = 0;
  ]])], [
    AC_DEFINE([HAVE_IPV6], [1], [Define if IPv6 is potentially available])
    AC_MSG_RESULT([yes])
  ], [
    AC_MSG_RESULT([no])
  ])
])

dnl ---------------------------------------------------------------
dnl Checks for headers
dnl ---------------------------------------------------------------
AC_CHECK_HEADERS([sys/sockio.h sys/param.h ifaddrs.h])

AC_CHECK_HEADERS([linux/netlink.h], [
  AC_DEFINE([HAVE_LINUX_NETLINK_H], [1],
            [Define if linux/netlink.h header exists])
])

dnl ---------------------------------------------------------------
dnl Checks for types
dnl
dnl On Darwin (MacOSX) socklen_t needs to be an int (see accept man
dnl page), on all other unix systems we need a size_t.
dnl ---------------------------------------------------------------
AC_MSG_CHECKING([for socklen_t])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
  #include <sys/types.h>
  #include <sys/socket.h>
]], [[socklen_t x;]])], [
  AC_MSG_RESULT([yes])
], [
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
    #include <sys/types.h>
    #include <sys/socket.h>
    int accept (int, struct sockaddr *, size_t *);
  ]], [[]])], [
    AC_MSG_RESULT([size_t])
    AC_DEFINE([socklen_t], [size_t],
              [Defined if system has no socklen_t type])
  ], [
    AC_MSG_RESULT([int])
    AC_DEFINE([socklen_t], [int],
              [Defined if system has no socklen_t type])
  ])
])

dnl ---------------------------------------------------------------
dnl Checks for structures
dnl ---------------------------------------------------------------
AC_CHECK_MEMBER([struct sockaddr.sa_len],
  [AC_DEFINE([HAVE_SOCKADDR_SA_LEN], [1],
             [Define if struct sockaddr has sa_len field])],,
  [[#include <sys/types.h>
    #include <sys/socket.h>]])

AC_MSG_CHECKING([for struct sockaddr_storage])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
  #include <sys/types.h>
  #include <sys/socket.h>
]], [[struct sockaddr_storage s; s;]])],
  [have_sockaddr_storage=yes], [have_sockaddr_storage=no])
if test "$have_sockaddr_storage" = yes; then
  AC_DEFINE([HAVE_SOCKADDR_STORAGE], [1],
            [Whether you have struct sockaddr_storage])
  AC_MSG_RESULT([yes])
  AC_MSG_CHECKING([for struct sockaddr_storage family field])
  gnet_sockaddr_family_field_name=""
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
    #include <sys/types.h>
    #include <sys/socket.h>
  ]], [[struct sockaddr_storage s; s.ss_family = AF_INET;]])],
    [gnet_sockaddr_family_field_name=ss_family])
  AS_IF([test -z "${gnet_sockaddr_family_field_name}"], [
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
      #include <sys/types.h>
      #include <sys/socket.h>
    ]], [[struct sockaddr_storage s; s.__ss_family = AF_INET;]])],
      [gnet_sockaddr_family_field_name=__ss_family])
  ])
  AS_IF([test -z "${gnet_sockaddr_family_field_name}"],
    [AC_MSG_ERROR([can't figure out 'family' field of struct sockaddr_storage])])
  AC_MSG_RESULT([$gnet_sockaddr_family_field_name])
else
  AC_MSG_RESULT([no])
  gnet_sockaddr_family_field_name=ss_family
fi
AC_DEFINE_UNQUOTED([GNET_SOCKADDR_FAMILY_FIELD_NAME],
  [$gnet_sockaddr_family_field_name],
  [Name of sockaddr_storage family field])

dnl ---------------------------------------------------------------
dnl Check if abstract sockets are supported
dnl ---------------------------------------------------------------
AC_LANG_PUSH([C])
AC_CACHE_CHECK([for abstract socket namespace availability],
  [ac_cv_gnet_have_abstract_sockets], [
    AC_RUN_IFELSE([AC_LANG_PROGRAM([[
      #include <sys/types.h>
      #include <stdlib.h>
      #include <string.h>
      #include <stdio.h>
      #include <sys/socket.h>
      #include <sys/un.h>
      #include <errno.h>
    ]], [[
      int listen_fd, len;
      struct sockaddr_un addr;

      listen_fd = socket (PF_UNIX, SOCK_STREAM, 0);
      if (listen_fd < 0) {
        perror ("socket() failed: ");
        exit (1);
      }
      memset (&addr, '\0', sizeof (addr));
      addr.sun_family = AF_UNIX;
      strcpy (addr.sun_path, "X/tmp/gnet-fake-socket-path-used-in-configure-test");
      len = SUN_LEN (&addr);
      addr.sun_path[0] = '\0';

      if (bind (listen_fd, (struct sockaddr*) &addr, len) < 0) {
        perror ("Abstract socket namespace bind() failed: ");
        exit (1);
      }
      exit (0);
    ]])],
    [ac_cv_gnet_have_abstract_sockets=yes],
    [ac_cv_gnet_have_abstract_sockets=no],
    [ac_cv_gnet_have_abstract_sockets=no])
])
AC_LANG_POP([C])

AS_IF([test x$ac_cv_gnet_have_abstract_sockets = xyes],
  [AC_DEFINE([HAVE_ABSTRACT_SOCKETS], [1], [Have abstract socket namespace])])

dnl ---------------------------------------------------------------
dnl Check for library functions
dnl ---------------------------------------------------------------
AC_CHECK_FUNC([getifaddrs],
  [AC_DEFINE([HAVE_GETIFADDRS], [1], [Define if getifaddrs() is available])])

dnl The user may be able to tell us if a function is thread-safe.  We
dnl know of no good way to test this programmatically.
AC_ARG_ENABLE([threadsafe-getaddrinfo],
  [AS_HELP_STRING([--enable-threadsafe-getaddrinfo],
                  [assume getaddrinfo() is threadsafe])])
AC_ARG_ENABLE([threadsafe-gethostbyname],
  [AS_HELP_STRING([--enable-threadsafe-gethostbyname],
                  [assume gethostbyname() is threadsafe])])

AC_CHECK_FUNC([getaddrinfo], [
  HAVE_GETADDRINFO=yes
  AC_DEFINE([HAVE_GETADDRINFO], [1],
            [Define if getaddrinfo() is available])
  AC_MSG_CHECKING([for threadsafe getaddrinfo()])
  if test "x$enable_threadsafe_getaddrinfo" = "xyes"; then
    HAVE_GETADDRINFO_THREADSAFE=yes
    AC_MSG_RESULT([yes])
    AC_DEFINE([HAVE_GETADDRINFO_THREADSAFE], [1],
              [Define if getaddrinfo is threadsafe])
  else
    AC_MSG_RESULT([no])
    if test "x$GTHREAD_CFLAGS" != "x"; then
      AC_DEFINE([HAVE_GETADDRINFO_GLIB_MUTEX], [1],
                [Define if getaddrinfo requires a mutex for thread-safety])
      HAVE_GETADDRINFO_GLIB_MUTEX=yes
    else
      AC_MSG_WARN([
You do not have a thread-safe getaddrinfo() or GLib threads.
Calls to getaddrinfo() (called by the GNet address functions) may
not be thread-safe.  If your system has a thread-safe version of
getaddrinfo(), pass --enable-threadsafe-getaddrinfo to configure.
])
    fi
  fi
], [
  AC_MSG_RESULT([no])
])

AC_MSG_CHECKING([for threadsafe gethostbyname()])
if test "x$enable_threadsafe_gethostbyname" = "xyes"; then
  AC_MSG_RESULT([yes])
  AC_DEFINE([HAVE_GETHOSTBYNAME_THREADSAFE], [1],
            [Define if gethostbyname is threadsafe])
  HAVE_GETHOSTBYNAME_THREADSAFE=yes
else
  AC_MSG_RESULT([no])
fi

dnl Look for a gethostbyname_r() we can use.
AC_MSG_CHECKING([for glibc gethostbyname_r])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <netdb.h>]], [[
  struct hostent result_buf;
  char buf[1024];
  struct hostent *result;
  int h_erropp;

  gethostbyname_r("localhost", &result_buf, buf, sizeof(buf),
                  &result, &h_erropp);
]])], [
  AC_MSG_RESULT([yes])
  AC_DEFINE([HAVE_GETHOSTBYNAME_R_GLIBC], [1],
            [Define if glibc variant of gethostbyname() is available])
  HAVE_GETHOSTBYNAME_R=yes
], [AC_MSG_RESULT([no])])

AC_MSG_CHECKING([for Solaris/Irix gethostbyname_r])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <netdb.h>]], [[
  struct hostent result;
  char buf[1024];
  int h_errnop;

  gethostbyname_r("localhost", &result, buf, sizeof(buf), &h_errnop);
]])], [
  AC_MSG_RESULT([yes])
  AC_DEFINE([HAVE_GETHOSTBYNAME_R_SOLARIS], [1],
            [Define if Solaris variant of gethostbyname() is available])
  HAVE_GETHOSTBYNAME_R=yes
], [AC_MSG_RESULT([no])])

AC_MSG_CHECKING([for HP-UX gethostbyname_r])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <netdb.h>]], [[
  struct hostent result;
  char buf[1024];

  gethostbyname_r("localhost", &result, buf);
]])], [
  AC_MSG_RESULT([yes])
  AC_DEFINE([HAVE_GETHOSTBYNAME_R_HPUX], [1],
            [Define if HPUX variant of gethostbyname() is available])
  HAVE_GETHOSTBYNAME_R=yes
], [AC_MSG_RESULT([no])])

if test -z "$HAVE_GETHOSTBYNAME_THREADSAFE" -a -z "$HAVE_GETHOSTBYNAME_R" -a "$GTHREAD_CFLAGS"; then
  AC_DEFINE([HAVE_GETHOSTBYNAME_R_GLIB_MUTEX], [1],
            [Define if gethostbyname requires a mutex for thread-safety])
  HAVE_GETHOSTBYNAME_R=yes
fi

if test -z "$HAVE_GETADDRINFO_THREADSAFE" -a -z "$HAVE_GETHOSTBYNAME_THREADSAFE" -a -z "$HAVE_GETHOSTBYNAME_R"; then
  AC_MSG_WARN([
You do not have a thread-safe gethostbyname_r() or GLib threads.
Calls to gethostbyname() (called by the GNet address functions) may
not be thread-safe.  If your system has a thread-safe version of
gethostbyname(), pass --enable-threadsafe-gethostbyname to configure.
])
fi

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

dnl just disable coverage stuff for now
AM_CONDITIONAL([GNET_GCOV_ENABLED], [test "xyes" = "xno"])

dnl ---------------------------------------------------------------
dnl Output
dnl ---------------------------------------------------------------
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
