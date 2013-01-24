AM_CPPFLAGS =      \
        -I$(top_srcdir)/libmaxminddb

noinst_PROGRAMS = version_t open_t lookup_t

version_t_LDADD = $(top_builddir)/libmaxminddb/libmaxminddb.la
version_t_SOURCES = version_t.c tap.c

open_t_LDADD = $(top_builddir)/libmaxminddb/libmaxminddb.la
open_t_SOURCES = open_t.c tap.c

lookup_t_LDADD = $(top_builddir)/libmaxminddb/libmaxminddb.la
lookup_t_SOURCES = lookup_t.c tap.c

lookup_t.lo lookup_t.o: lookup_t.c

version_t.lo version_t.o: version_t.c

open_t.lo open_t.o: open_t.c

tap.lo tap.o: tap.c

