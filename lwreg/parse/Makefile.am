noinst_LTLIBRARIES = libregparse.la

libregparse_la_SOURCES = \
    regio.c \
    reglex.c \
    regiconv.c \
    regparse.c

libregparse_la_CPPFLAGS = \
    $(AM_CPPFLAGS)        \
    @LWBASE_INCLUDES@   \
    @LWMSG_INCLUDES@

libregparse_la_LIBADD = \
    @top_builddir@/common/libregcommon.la

libregparse_la_LDFLAGS = @LWBASE_LDFLAGS@
