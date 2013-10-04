include $(top_srcdir)/common.mk

AM_LDFLAGS = $(top_builddir)/src/libmaxminddb.la

lib_LTLIBRARIES = libmmdbtest.la
libmmdbtest_la_SOURCES = maxminddb_test_helper.c libtap/tap.c

check_PROGRAMS = \
	 aget_value_t basic_lookup_t data_entry_list_t data_types_t \
	 dump_t metadata_t no_map_get_value_t read_node_t version_t

TESTS = $(check_PROGRAMS) mmdblookup_t.pl

LDADD = libmmdbtest.la
