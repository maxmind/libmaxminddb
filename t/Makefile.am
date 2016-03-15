include $(top_srcdir)/common.mk

AM_LDFLAGS = $(top_builddir)/src/libmaxminddb.la

noinst_LTLIBRARIES = libmmdbtest.la
libmmdbtest_la_SOURCES = maxminddb_test_helper.c libtap/tap.c

check_PROGRAMS = \
	bad_pointers_t basic_lookup_t data_entry_list_t data_types_t \
	dump_t get_value_t get_value_pointer_bug_t ipv4_start_cache_t \
	ipv6_lookup_in_ipv4_t metadata_t metadata_pointers_t          \
	no_map_get_value_t read_node_t threads_t version_t

threads_t_CFLAGS = $(CFLAGS) -pthread

TESTS = $(check_PROGRAMS) compile_c++_t.pl mmdblookup_t.pl

LDADD = libmmdbtest.la
