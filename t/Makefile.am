AM_CPPFLAGS = -I$(top_srcdir)/include

AM_LDFLAGS = $(top_builddir)/src/libmaxminddb.la

lib_LTLIBRARIES = libmmdbtest.la
libmmdbtest_la_SOURCES = maxminddb_test_helper.c libtap/tap.c

check_PROGRAMS = \
	 basic_lookup_t data_entry_list_t data_types_t \
	 dump_t metadata_t no_map_get_value_t version_t

TESTS = $(check_PROGRAMS)

LDADD = libmmdbtest.la

