# Configure paths for Gnet
# A hacked up version of Owen Taylor's glib-2.0.m4 (Copyright 1997-2001)

dnl AM_PATH_GNET_2_0([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND [, MODULES]]]])
dnl Test for GNET, and define GNET_CFLAGS and GNET_LIBS, if gmodule, gobject or 
dnl gthread is specified in MODULES, pass to pkg-config
dnl
AC_DEFUN([AM_PATH_GNET_2_0],
[dnl 
dnl Get the cflags and libraries from pkg-config
dnl
AC_ARG_ENABLE(gnettest, [  --disable-gnettest      do not try to compile and run a test GNET program],
		    , enable_gnettest=yes)

  pkg_config_args=gnet-2.0

  AC_PATH_PROG(PKG_CONFIG, pkg-config, no)

  no_gnet=""

  if test x$PKG_CONFIG != xno ; then
    if $PKG_CONFIG --atleast-pkgconfig-version 0.7 ; then
      :
    else
      echo *** pkg-config too old; version 0.7 or better required.
      no_gnet=yes
      PKG_CONFIG=no
    fi
  else
    no_gnet=yes
  fi

  min_gnet_version=ifelse([$1], , 2.0.0, $1)
  AC_MSG_CHECKING(for GNET - version >= $min_gnet_version)

  if test x$PKG_CONFIG != xno ; then
    ## don't try to run the test against uninstalled libtool libs
    if $PKG_CONFIG --uninstalled $pkg_config_args; then
	  echo "Will use uninstalled version of GNet found in PKG_CONFIG_PATH"
	  enable_gnettest=no
    fi

    if $PKG_CONFIG --atleast-version $min_gnet_version $pkg_config_args; then
	  :
    else
	  no_gnet=yes
    fi
  fi

  if test x"$no_gnet" = x ; then
    GNET_CFLAGS=`$PKG_CONFIG --cflags $pkg_config_args`
    GNET_LIBS=`$PKG_CONFIG --libs $pkg_config_args`
    gnet_config_major_version=`$PKG_CONFIG --modversion gnet-2.0 | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    gnet_config_minor_version=`$PKG_CONFIG --modversion gnet-2.0 | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    gnet_config_micro_version=`$PKG_CONFIG --modversion gnet-2.0 | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_gnettest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $GNET_CFLAGS"
      LIBS="$GNET_LIBS $LIBS"
dnl
dnl Now check if the installed GNET is sufficiently new. (Also sanity
dnl checks the results of pkg-config to some extent)
dnl
      rm -f conf.gnettest
      AC_TRY_RUN([
#include <gnet.h>
#include <stdio.h>
#include <stdlib.h>

int 
main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.gnettest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = g_strdup("$min_gnet_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_gnet_version");
     exit(1);
   }

  if ((gnet_major_version != $gnet_config_major_version) ||
      (gnet_minor_version != $gnet_config_minor_version) ||
      (gnet_micro_version != $gnet_config_micro_version))
    {
      printf("\n*** 'pkg-config --modversion gnet-2.0' returned %d.%d.%d, but GNET (%d.%d.%d)\n", 
             $gnet_config_major_version, $gnet_config_minor_version, $gnet_config_micro_version,
             gnet_major_version, gnet_minor_version, gnet_micro_version);
      printf ("*** was found! If pkg-config was correct, then it is best\n");
      printf ("*** to remove the old version of GNet. You may also be able to fix the error\n");
      printf("*** by modifying your LD_LIBRARY_PATH enviroment variable, or by editing\n");
      printf("*** /etc/ld.so.conf. Make sure you have run ldconfig if that is\n");
      printf("*** required on your system.\n");
      printf("*** If pkg-config was wrong, set the environment variable PKG_CONFIG_PATH\n");
      printf("*** to point to the correct configuration files\n");
    } 
  else if ((gnet_major_version != GNET_MAJOR_VERSION) ||
	   (gnet_minor_version != GNET_MINOR_VERSION) ||
           (gnet_micro_version != GNET_MICRO_VERSION))
    {
      printf("*** GNET header files (version %d.%d.%d) do not match\n",
	     GNET_MAJOR_VERSION, GNET_MINOR_VERSION, GNET_MICRO_VERSION);
      printf("*** library (version %d.%d.%d)\n",
	     gnet_major_version, gnet_minor_version, gnet_micro_version);
    }
  else
    {
      if ((gnet_major_version > major) ||
        ((gnet_major_version == major) && (gnet_minor_version > minor)) ||
        ((gnet_major_version == major) && (gnet_minor_version == minor) && (gnet_micro_version >= micro)))
      {
        return 0;
       }
     else
      {
        printf("\n*** An old version of GNET (%d.%d.%d) was found.\n",
               gnet_major_version, gnet_minor_version, gnet_micro_version);
        printf("*** You need a version of GNET newer than %d.%d.%d. The latest version of\n",
	       major, minor, micro);
        printf("*** GNET is always available from ftp://ftp.gtk.org.\n");
        printf("***\n");
        printf("*** If you have already installed a sufficiently new version, this error\n");
        printf("*** probably means that the wrong copy of the pkg-config shell script is\n");
        printf("*** being found. The easiest way to fix this is to remove the old version\n");
        printf("*** of GNET, but you can also set the PKG_CONFIG environment to point to the\n");
        printf("*** correct copy of pkg-config. (In this case, you will have to\n");
        printf("*** modify your LD_LIBRARY_PATH enviroment variable, or edit /etc/ld.so.conf\n");
        printf("*** so that the correct libraries are found at run-time))\n");
      }
    }
  return 1;
}
],, no_gnet=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_gnet" = x ; then
     AC_MSG_RESULT(yes (version $gnet_config_major_version.$gnet_config_minor_version.$gnet_config_micro_version))
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$PKG_CONFIG" = "no" ; then
       echo "*** A new enough version of pkg-config was not found."
       echo "*** See http://www.freedesktop.org/software/pkgconfig/"
     else
       if test -f conf.gnettest ; then
        :
       else
          echo "*** Could not run GNET test program, checking why..."
          ac_save_CFLAGS="$CFLAGS"
          ac_save_LIBS="$LIBS"
          CFLAGS="$CFLAGS $GNET_CFLAGS"
          LIBS="$LIBS $GNET_LIBS"
          AC_TRY_LINK([
#include <gnet.h>
#include <stdio.h>
],      [ return ((gnet_major_version) || (gnet_minor_version) || (gnet_micro_version)); ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding GNET or finding the wrong"
          echo "*** version of GNET. If it is not finding GNET, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH" ],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means GNET is incorrectly installed."])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     GNET_CFLAGS=""
     GNET_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(GNET_CFLAGS)
  AC_SUBST(GNET_LIBS)
  rm -f conf.gnettest
])
