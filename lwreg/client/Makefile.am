lib_LTLIBRARIES = libregclient.la

libregclient_la_SOURCES = \
    clientipc.c \
    regclient.c \
    regntclient.c

libregclient_la_CPPFLAGS = \
    $(AM_CPPFLAGS)         \
    @LWBASE_INCLUDES@    \
    @LWMSG_INCLUDES@

libregclient_la_LIBADD = \
	@top_builddir@/common/libregcommon.la
