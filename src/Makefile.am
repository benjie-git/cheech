## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-I$(srcdir)/gnet-2.0 \
	$(GLIBMM_CFLAGS) \
	$(GUI_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTHREAD_CFLAGS) \
	$(MACOS_APP_CFLAGS) \
	-D_REENTRANT \
	-DGNET_EXPERIMENTAL

AM_CFLAGS = -Wall
AM_CXXFLAGS = -Wall -std=c++17

# gtkmm flags, only set when the GUI is built (--disable-gui leaves
# them empty so the server/bot binaries need no GTK).
if ENABLE_GUI
GUI_CFLAGS = $(GTKMM_CFLAGS)
GUI_LIBS = $(GTKMM_LIBS)
else
GUI_CFLAGS =
GUI_LIBS =
endif

bin_PROGRAMS = cheechd cheechbot cheechwebd
if ENABLE_GUI
bin_PROGRAMS += cheech
endif

cheech_LDADD = \
	$(GUI_LIBS) $(GLIBMM_LIBS) $(GTHREAD_LIBS) $(GLIB_LIBS) -lpthread $(MACOS_APP_LIBS)
cheechd_LDADD = \
	$(GLIBMM_LIBS) $(GTHREAD_LIBS) $(GLIB_LIBS) -lpthread
cheechbot_LDADD = \
	$(GLIBMM_LIBS) $(GTHREAD_LIBS) $(GLIB_LIBS) -lpthread
cheechwebd_LDADD = \
	$(GLIBMM_LIBS) $(GTHREAD_LIBS) $(GLIB_LIBS) -lpthread

gnet_sources = \
	gnet-2.0/base64.c \
	gnet-2.0/base64.h \
	gnet-2.0/conn-http.c \
	gnet-2.0/conn-http.h \
	gnet-2.0/conn.c \
	gnet-2.0/conn.h \
	gnet-2.0/gnet-private.c \
	gnet-2.0/gnet-private.h \
	gnet-2.0/gnet.c \
	gnet-2.0/gnet.h \
	gnet-2.0/inetaddr.c \
	gnet-2.0/inetaddr.h \
	gnet-2.0/iochannel.c \
	gnet-2.0/iochannel.h \
	gnet-2.0/ipv6.c \
	gnet-2.0/ipv6.h \
	gnet-2.0/mcast.c \
	gnet-2.0/mcast.h \
	gnet-2.0/md5.c \
	gnet-2.0/md5.h \
	gnet-2.0/pack.c \
	gnet-2.0/pack.h \
	gnet-2.0/server.c \
	gnet-2.0/server.h \
	gnet-2.0/sha.c \
	gnet-2.0/sha.h \
	gnet-2.0/socks-private.c \
	gnet-2.0/socks-private.h \
	gnet-2.0/socks.c \
	gnet-2.0/socks.h \
	gnet-2.0/tcp.c \
	gnet-2.0/tcp.h \
	gnet-2.0/udp.c \
	gnet-2.0/udp.h \
	gnet-2.0/unix.c \
	gnet-2.0/unix.h \
	gnet-2.0/uri.c \
	gnet-2.0/uri.h \
	gnet-2.0/usagi_ifaddrs.c \
	gnet-2.0/usagi_ifaddrs.h

bot_sources = \
	bot_base.cc \
	bot_base.hh \
	bot_random.cc \
	bot_random.hh \
	bot_simple.cc \
	bot_simple.hh \
	bot_lookahead.cc \
	bot_lookahead.hh \
	bot_friendly.cc \
	bot_friendly.hh \
	bot_mean.cc \
	bot_mean.hh

cheech_SOURCES = \
	cheech.cc \
	about_win.cc \
	about_win.hh \
	about_win_glade.cc \
	about_win_glade.hh \
	$(bot_sources) \
	color_win.cc \
	color_win.hh \
	color_win_glade.cc \
	color_win_glade.hh \
	name_win.cc \
	name_win.hh \
	name_win_glade.cc \
	name_win_glade.hh \
	setup_bot_win.cc \
	setup_bot_win.hh \
	setup_bot_win_glade.cc \
	setup_bot_win_glade.hh \
	game_board.cc \
	game_board.hh \
	main_win.cc \
	main_win.hh \
	main_win_glade.cc \
	main_win_glade.hh \
	new_game_win.cc \
	new_game_win.hh \
	new_game_win_glade.cc \
	new_game_win_glade.hh \
	setup_game_win.cc \
	setup_game_win.hh \
	setup_game_win_glade.cc \
	setup_game_win_glade.hh \
	help_win.cc \
	help_win.hh \
	help_win_glade.cc \
	help_win_glade.hh \
	gnet_conn.cc \
	gnet_conn.hh \
	gnet_server.cc \
	gnet_server.hh \
	utility.cc \
	utility.hh \
	game_images.cc \
	game_images.hh \
	game_server.cc \
	game_server.hh \
	game_client.cc \
	game_client.hh \
	game_hole.cc \
	game_hole.hh \
	game_view.cc \
	game_view.hh \
	game_view_hole.cc \
	game_view_hole.hh \
	prefs.cc \
	prefs.hh \
	ajax_server.cc \
	ajax_server.hh \
	ajax_server_conn.cc \
	ajax_server_conn.hh \
	$(gnet_sources)

# The About-window icon is embedded in the GUI binary as a GResource.
nodist_cheech_SOURCES = cheech_resources.c
CLEANFILES = cheech_resources.c

cheech_resources.c: $(top_srcdir)/pixmaps/cheech.gresource.xml $(top_srcdir)/pixmaps/cheech.png
	$(GLIB_COMPILE_RESOURCES) --generate-source --target=$@ \
		--sourcedir=$(top_srcdir)/pixmaps \
		$(top_srcdir)/pixmaps/cheech.gresource.xml

cheechd_SOURCES = \
	cheechd.cc \
	$(bot_sources) \
	game_images.cc \
	game_images.hh \
	gnet_conn.cc \
	gnet_conn.hh \
	gnet_server.cc \
	gnet_server.hh \
	utility.cc \
	utility.hh \
	game_server.cc \
	game_server.hh \
	game_client.cc \
	game_client.hh \
	game_board.cc \
	game_board.hh \
	game_hole.cc \
	game_hole.hh \
	prefs.cc \
	prefs.hh \
	ajax_server.cc \
	ajax_server.hh \
	ajax_server_conn.cc \
	ajax_server_conn.hh \
	$(gnet_sources)

cheechbot_SOURCES = \
	cheechbot.cc \
	$(bot_sources) \
	game_board.cc \
	game_board.hh \
	game_images.cc \
	game_images.hh \
	gnet_conn.cc \
	gnet_conn.hh \
	utility.cc \
	utility.hh \
	game_client.cc \
	game_client.hh \
	game_hole.cc \
	game_hole.hh \
	$(gnet_sources)

cheechwebd_SOURCES = \
	cheechwebd.cc \
	$(bot_sources) \
	game_images.cc \
	game_images.hh \
	gnet_conn.cc \
	gnet_conn.hh \
	gnet_server.cc \
	gnet_server.hh \
	utility.cc \
	utility.hh \
	game_client.cc \
	game_client.hh \
	game_board.cc \
	game_board.hh \
	game_hole.cc \
	game_hole.hh \
	prefs.cc \
	prefs.hh \
	ajax_server.cc \
	ajax_server.hh \
	ajax_server_conn.cc \
	ajax_server_conn.hh \
	$(gnet_sources)
