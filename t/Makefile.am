AM_CPPFLAGS =      \
        -I$(top_srcdir)/libmaxminddb

AM_LDFLAGS = $(top_builddir)/libmaxminddb/libmaxminddb.la

check_PROGRAMS = data_types_t
# version_t open_t lookup_t dump_t dump_meta_t endian_size_t

TESTS = $(check_PROGRAMS)

data_types_t_SOURCES = data_types_t.c tap.c MMDB_test_helper.c

