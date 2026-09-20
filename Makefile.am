## Process this file with automake to produce Makefile.in

SUBDIRS = src pixmaps ajax

docdir = ${prefix}/doc/cheech
dist_doc_DATA = \
	README \
	COPYING \
	AUTHORS \
	ChangeLog \
	INSTALL \
	NEWS \
	TODO

EXTRA_DIST = m4/ChangeLog macosx/make-bundle.sh

# Build a relocatable Mac OS X .app bundle containing the GUI.
# Requires configure --enable-macos-app and dylibbundler.
.PHONY: bundle
bundle: all
	$(SHELL) "$(abs_top_srcdir)/macosx/make-bundle.sh" \
		--srcdir "$(abs_top_srcdir)" \
		--builddir "$(abs_top_builddir)" \
		--name "Cheech" \
		--version "$(PACKAGE_VERSION)"

# Copy all the spec files. Of course, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done
