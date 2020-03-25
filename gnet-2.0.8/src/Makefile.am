## Process this file with automake to produce Makefile.in

if OS_WIN32
no_undefined = -no-undefined
lws2_32 = -lws2_32
endif

@SET_MAKE@

INCLUDES = -DG_LOG_DOMAIN=\"GNet\" @GNET_DEBUG_FLAGS@ \
	$(GLIB_CFLAGS) $(GTHREAD_CFLAGS) 

gnetincludedir = $(includedir)/gnet-$(GNET_MAJOR_VERSION).$(GNET_MINOR_VERSION)

lib_LTLIBRARIES = libgnet-2.0.la

libgnet_2_0_la_LDFLAGS = \
	$(no_undefined) \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	$(GLIB_LIBS) $(GTHREAD_LIBS) $(lws2_32)

libgnet_2_0_la_SOURCES = 	\
	gnet.c			\
	gnet-private.c		\
	ipv6.c			\
	inetaddr.c		\
	mcast.c			\
	tcp.c			\
	unix.c			\
	udp.c			\
	iochannel.c		\
	socks.c			\
	socks-private.c		\
	md5.c			\
	sha.c			\
	pack.c			\
	uri.c			\
	conn.c			\
	conn-http.c             \
	server.c		\
	usagi_ifaddrs.c		\
	base64.c

gnetinclude_HEADERS = 		\
	gnet.h			\
	ipv6.h			\
        inetaddr.h              \
        mcast.h           	\
	tcp.h			\
	unix.h			\
        udp.h			\
	iochannel.h		\
	socks.h			\
	md5.h			\
	sha.h			\
	pack.h			\
	uri.h			\
	conn.h			\
	conn-http.h             \
	server.h		\
	base64.h
