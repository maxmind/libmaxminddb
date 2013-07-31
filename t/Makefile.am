AM_CPPFLAGS = -I$(top_srcdir)/libmaxminddb

AM_LDFLAGS = $(top_builddir)/libmaxminddb/libmaxminddb.la

lib_LTLIBRARIES = libmmdbtest.la
libmmdbtest_la_SOURCES = tap.c MMDB_test_helper.c

check_PROGRAMS = data_types_t metadata_t
# version_t open_t lookup_t dump_t dump_meta_t endian_size_t

TESTS = $(check_PROGRAMS)

data_types_t_LDADD = libmmdbtest.la
metadata_t_LDADD = libmmdbtest.la
