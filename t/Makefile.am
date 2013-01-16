AM_CPPFLAGS =      \
        -I$(top_srcdir)/libmaxminddb

bin_PROGRAMS = open_t

open_t_LDADD = $(top_builddir)/libmaxminddb/libmaxminddb.la
open_t_SOURCES = open_t.c tap.c

open_t.lo open_t.o: open_t.c

tap.lo tap.o: tap.c

