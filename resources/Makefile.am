## Process this file with automake to produce Makefile.in

pixmaps1dir = $(datadir)/pixmaps
dist_pixmaps1_DATA = cheech.png

pixmaps2dir = $(datadir)/pixmaps/$(PACKAGE)
dist_pixmaps2_DATA = cheech.png

desktopdir = $(datadir)/applications
hicoloricondir = $(datadir)/icons/hicolor/128x128/apps

# Menu entry + themed icon only make sense when the GUI is built;
# server/bot-only installs should not advertise a cheech GUI.
if ENABLE_GUI
dist_desktop_DATA = cheech.desktop
dist_hicoloricon_DATA = cheech.png
endif

EXTRA_DIST = cheech.gresource.xml acknowledgements.txt

install-data-hook:
	-update-desktop-database "$(DESTDIR)$(desktopdir)"
	-gtk-update-icon-cache -qf -t "$(DESTDIR)$(datadir)/icons/hicolor"
