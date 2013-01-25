AM_CPPFLAGS =      \
        -I$(top_srcdir)/libmaxminddb

noinst_PROGRAMS = open_t lookup_t

open_t_LDADD = $(top_builddir)/libmaxminddb/libmaxminddb.la
open_t_SOURCES = open_t.c tap.c

lookup_t_LDADD = $(top_builddir)/libmaxminddb/libmaxminddb.la
lookup_t_SOURCES = lookup_t.c tap.c

lookup_t.lo lookup_t.o: lookup_t.c

open_t.lo open_t.o: open_t.c

tap.lo tap.o: tap.c

