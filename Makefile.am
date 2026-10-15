## Process this file with automake to produce Makefile.in

SUBDIRS = src pixmaps ajax

docdir = ${prefix}/doc/cheech
dist_doc_DATA = \
	README.md \
	COPYING \
	AUTHORS \
	ChangeLog \
	INSTALL \
	NEWS \
	TODO

EXTRA_DIST = m4/ChangeLog macos/make-bundle.sh windows/make-bundle.sh

# Build a relocatable Mac OS X .app bundle containing the GUI.
# Requires configure --enable-macos-app and dylibbundler.
# On MinGW/MSYS2 hosts the same target builds a portable Windows
# directory instead (see windows/make-bundle.sh).
if WIN32_HOST
.PHONY: bundle
bundle: all
	$(SHELL) "$(abs_top_srcdir)/windows/make-bundle.sh" \
		--srcdir "$(abs_top_srcdir)" \
		--builddir "$(abs_top_builddir)" \
		--name "Cheech" \
		--version "$(PACKAGE_VERSION)"
else
.PHONY: bundle
bundle: all
	$(SHELL) "$(abs_top_srcdir)/macos/make-bundle.sh" \
		--srcdir "$(abs_top_srcdir)" \
		--builddir "$(abs_top_builddir)" \
		--name "Cheech" \
		--version "$(PACKAGE_VERSION)"
endif

# Copy all the spec files. Of course, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done
