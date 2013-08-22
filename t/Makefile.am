AM_CPPFLAGS = -I$(top_srcdir)/include

AM_LDFLAGS = $(top_builddir)/src/libmaxminddb.la

lib_LTLIBRARIES = libmmdbtest.la
libmmdbtest_la_SOURCES = MMDB_test_helper.c libtap/tap.c

check_PROGRAMS = basic_lookup_t data_types_t metadata_t version_t

TESTS = $(check_PROGRAMS)

LDADD = libmmdbtest.la

