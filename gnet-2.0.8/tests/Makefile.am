## Process this file with automake to produce Makefile.in

@SET_MAKE@

if ENABLE_NETWORK_TESTS
  NETWORK_TESTS = client_server_test.pl dns_test.pl
else
  NETWORK_TESTS =
endif

TESTS = $(NETWORK_TESTS)
check_PROGRAMS = 
check_SCRIPTS= $(NETWORK_TESTS)
EXTRA_SCRIPTS = client_server_test.pl dns_test.pl
CLEANFILES = $(check_PROGRAMS) .client*out .server*out .client*diff .server*diff

INCLUDES = -I$(top_srcdir)/src $(GLIB_CFLAGS)
LDADD = $(GLIB_LIBS) $(GTHREAD_LIBS) $(top_builddir)/src/libgnet-$(GNET_MAJOR_VERSION).$(GNET_MINOR_VERSION).la

if HAVE_CHECK
SUBDIRS_CHECK = check
else
SUBDIRS_CHECK =
endif

SUBDIRS = $(SUBDIRS_CHECK)

DIST_SUBDIRS = check

