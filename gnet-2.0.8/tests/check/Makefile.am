##############################################################################
# The following is from GStreamer's magnificent check.mak
##############################################################################

clean-local-check:
	for i in `find . -name ".libs" -type d`; do \
	  rm -rf $$i; \
	done

if HAVE_VALGRIND
# hangs spectacularly on some machines, so let's not do this by default yet
check-valgrind:
	make valgrind
else
check-valgrind:
	@true
endif

LOOPS = 10

# run any given test by running make test.check
# if the test fails, run it again at at least debug level 2
%.check: %
	@$(TESTS_ENVIRONMENT)					\
	CK_DEFAULT_TIMEOUT=20					\
	$* ||							\
	$(TESTS_ENVIRONMENT)					\
	GNET_DEBUG=$$GNET_DEBUG,*:2				\
	CK_DEFAULT_TIMEOUT=20					\
	$*

# run any given test in a loop
%.torture: %
	@for i in `seq 1 $(LOOPS)`; do				\
	$(TESTS_ENVIRONMENT)					\
	CK_DEFAULT_TIMEOUT=20					\
	$*; done

# run any given test in an infinite loop
%.forever: %
	@while true; do						\
	$(TESTS_ENVIRONMENT)					\
	CK_DEFAULT_TIMEOUT=20					\
	$* || break; done

# valgrind any given test by running make test.valgrind
%.valgrind: %
	$(TESTS_ENVIRONMENT)					\
	CK_DEFAULT_TIMEOUT=360					\
	G_SLICE=always-malloc					\
	libtool --mode=execute					\
	$(VALGRIND_PATH) -q					\
	$(foreach s,$(SUPPRESSIONS),--suppressions=$(s))	\
	--tool=memcheck --leak-check=full --trace-children=yes	\
	--leak-resolution=high --num-callers=20			\
	./$* 2>&1 | tee valgrind.log
	@if grep "==" valgrind.log > /dev/null 2>&1; then	\
	    rm valgrind.log;					\
	    exit 1;						\
	fi
	@rm valgrind.log
	
# valgrind any given test and generate suppressions for it
%.valgrind.gen-suppressions: %
	$(TESTS_ENVIRONMENT)					\
	CK_DEFAULT_TIMEOUT=360					\
	G_SLICE=always-malloc					\
	libtool --mode=execute					\
	$(VALGRIND_PATH) -q 					\
	$(foreach s,$(SUPPRESSIONS),--suppressions=$(s))	\
	--tool=memcheck --leak-check=full --trace-children=yes	\
	--leak-resolution=high --num-callers=20			\
	--gen-suppressions=all					\
	./$* 2>&1 | tee suppressions.log
	
# valgrind any given test until failure by running make test.valgrind-forever
%.valgrind-forever: %
	@while make $*.valgrind; do				\
	  true; done

# gdb any given test by running make test.gdb
%.gdb: %
	$(TESTS_ENVIRONMENT)					\
	CK_FORK=no						\
	libtool --mode=execute					\
	gdb $*

# torture tests
torture: $(TESTS)
	@echo "Torturing tests ..."
	for i in `seq 1 $(LOOPS)`; do				\
		make check ||					\
		(echo "Failure after $$i runs"; exit 1) ||	\
		exit 1;						\
	done
	@banner="All $(LOOPS) loops passed";			\
	dashes=`echo "$$banner" | sed s/./=/g`;			\
	echo $$dashes; echo $$banner; echo $$dashes

# forever tests
forever: $(TESTS)
	@echo "Forever tests ..."
	while true; do						\
		make check ||					\
		(echo "Failure"; exit 1) ||			\
		exit 1;						\
	done

# valgrind all tests
valgrind: $(TESTS)
	@echo "Valgrinding tests ..."
	@failed=0;							\
	for t in $(filter-out $(VALGRIND_TESTS_DISABLE),$(TESTS)); do	\
		make $$t.valgrind;					\
		if test "$$?" -ne 0; then                               \
                        echo "Valgrind error for test $$t";		\
			failed=`expr $$failed + 1`;			\
			whicht="$$whicht $$t";				\
                fi;							\
	done;								\
	if test "$$failed" -ne 0; then					\
		echo "$$failed tests had leaks or errors under valgrind:";	\
		echo "$$whicht";					\
		false;							\
	fi

help:
	@echo "make check                         -- run all checks"
	@echo "make torture                       -- run all checks $(LOOPS) times"
	@echo "make (dir)/(test).check            -- run the given check once"
	@echo "make (dir)/(test).forever          -- run the given check forever"
	@echo "make (dir)/(test).torture          -- run the given check $(LOOPS) times"
	@echo
	@echo "make (dir)/(test).gdb              -- start up gdb for the given test"
	@echo
	@echo "make valgrind                      -- valgrind all tests"
	@echo "make (dir)/(test).valgrind         -- valgrind the given test"
	@echo "make (dir)/(test).valgrind-forever -- valgrind the given test forever"
	@echo "make (dir)/(test).valgrind.gen-suppressions -- generate suppressions"
	@echo "                                               and save to suppressions.log"


##############################################################################
# Our helper lib for unit tests
##############################################################################

noinst_LIBRARIES = libgnetcheck.a

libgnetcheck_a_SOURCES = gnetcheck.c gnetcheck.h

##############################################################################
# Details for GNet tests
##############################################################################

TESTS_ENVIRONMENT = 

# override to _not_ install the test plugins
install-pluginLTLIBRARIES:

# the core dumps of some machines have PIDs appended and profiling data
CLEANFILES = core core.* *.gcno *.gcda

SUPPRESSIONS = $(top_srcdir)/tests/check/gnet-valgrind.supp

EXTRA_DIST = $(SUPPRESSIONS)

clean-local: clean-local-check

if ENABLE_NETWORK_TESTS
  NETWORK_CHECKS = gnet/gnettcpsocket
else
  NETWORK_CHECKS =
endif

check_PROGRAMS = \
	$(NETWORK_CHECKS)  \
	gnet/gnetabi       \
	gnet/gnetbase64    \
	gnet/gnetconn      \
	gnet/gnetconnhttp  \
	gnet/gnethash      \
	gnet/gnetinetaddr  \
	gnet/gnetipv6      \
	gnet/gnetmisc      \
	gnet/gnetpack      \
	gnet/gnetunpack    \
	gnet/gneturi

# failing tests
noinst_PROGRAMS =

TESTS = $(check_PROGRAMS)

noinst_HEADERS =              \
	gnet/struct_i386.h    \
	gnet/struct_ppc32.h   \
	gnet/struct_x86_64.h

AM_CFLAGS = \
	-I$(top_srcdir)/src -I$(top_builddir)      \
	$(GLIB_CFLAGS)                             \
	$(CHECK_CFLAGS)
LDADD = \
	$(top_builddir)/tests/check/libgnetcheck.a \
	$(top_builddir)/src/libgnet-2.0.la         \
	$(GLIB_LIBS)                               \
	$(CHECK_LIBS)


# valgrind testing
# these just need valgrind fixing, period
VALGRIND_TO_FIX = 

# these should never be run with valgrind
VALGRIND_IGNORE = 

# these need fixing because the threads cause segfaults under valgrind
TESTS_THREADED =

VALGRIND_TESTS_DISABLE =   \
	$(TESTS_THREADED)  \
	$(VALGRIND_IGNORE) \
	$(VALGRIND_TO_FIX)

# directories with source files for coverage tests (relativ to toplevel dir)
COVERAGE_DIRS = \
	src

COVERAGE_FILES = $(foreach dir,$(COVERAGE_DIRS),$(wildcard $(top_builddir)/$(dir)/*.gcov))
COVERAGE_FILES_REL = $(subst $(top_builddir)/,,$(COVERAGE_FILES))
COVERAGE_OUT_FILES = $(foreach dir,$(COVERAGE_DIRS),$(wildcard $(top_builddir)/$(dir)/*.gcov.out))
COVERAGE_OUT_FILES_REL = $(subst $(top_builddir)/,,$(COVERAGE_OUT_FILES))

debug:
	echo $(COVERAGE_FILES)
	echo $(COVERAGE_FILES_REL)

.PHONY: coverage
if GNET_GCOV_ENABLED
coverage:
	for file in `find $(top_builddir) -name '*.gcda'`; do rm $$file; done
	make check
	make coverage-report
else
coverage:
	echo "You need to configure with --enable-gcov to get coverage data"
	exit 1
endif

coverage-report:
	rm -r coverage
	for dir in $(COVERAGE_DIRS); do 				\
	  mkdir -p coverage/$$dir; 					\
	  make -C $(top_builddir)/$$dir gcov;				\
        done
	for dir in $(COVERAGE_DIRS); do					\
	    files="`ls $(top_builddir)/$$dir/*.gcov.out 2> /dev/null`"; \
          if test ! -z "$$files"; then					\
	    perl $(top_srcdir)/common/coverage/coverage-report.pl 	\
	      $(top_builddir)/$$dir/*.gcov.out >			\
	      coverage/$$dir/index.xml;					\
	    xsltproc $(top_srcdir)/common/coverage/coverage-report.xsl	\
	      coverage/$$dir/index.xml > coverage/$$dir/index.html;	\
	  fi; \
        done
	
	for file in $(COVERAGE_FILES_REL); do				\
	  echo Generating coverage/$$file.html; 			\
	  perl $(top_srcdir)/common/coverage/coverage-report-entry.pl   \
	    $(top_builddir)/$$file > coverage/$$file.html; 		\
	done


