AM_CPPFLAGS =      \
        -I$(top_srcdir)/libmaxminddb

noinst_PROGRAMS = version_t open_t lookup_v4_t lookup_t dump_t dump_meta_t endian_size_t

version_t_LDADD = $(top_builddir)/libmaxminddb/libmaxminddb.la
version_t_SOURCES = version_t.c tap.c test_helper.c

open_t_LDADD = $(top_builddir)/libmaxminddb/libmaxminddb.la
open_t_SOURCES = open_t.c tap.c test_helper.c

lookup_v4_t_LDADD = $(top_builddir)/libmaxminddb/libmaxminddb.la
lookup_v4_t_SOURCES = lookup_v4_t.c tap.c

lookup_t_LDADD = $(top_builddir)/libmaxminddb/libmaxminddb.la
lookup_t_SOURCES = lookup_t.c tap.c test_helper.c

dump_t_LDADD = $(top_builddir)/libmaxminddb/libmaxminddb.la
dump_t_SOURCES = dump_t.c tap.c  test_helper.c

dump_meta_t_LDADD = $(top_builddir)/libmaxminddb/libmaxminddb.la
dump_meta_t_SOURCES = dump_meta_t.c tap.c test_helper.c

endian_size_t_LDADD = $(top_builddir)/libmaxminddb/libmaxminddb.la
endian_size_t_SOURCES = endian_size_t.c tap.c test_helper.c

lookup_t.lo lookup_t.o: lookup_t.c

lookup_v4_t.lo lookup_v4_t.o: lookup_v4_t.c

version_t.lo version_t.o: version_t.c

open_t.lo open_t.o: open_t.c

dump_t.lo dump_t.o: dump_t.c

endian_size_t.lo endian_size_t.o: endian_size_t.c

dump_meta_t.lo dump_meta_t.o: dump_meta_t.c

tap.lo tap.o: tap.c

test_helper.lo test_helper.o: test_helper.c

