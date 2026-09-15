include $(top_srcdir)/common.mk

AM_LDFLAGS = $(top_builddir)/src/libmaxminddb.la
CFLAGS += -I$(top_srcdir)/src $(CMOCKA_CFLAGS)

noinst_LTLIBRARIES = libmmdbtest.la
libmmdbtest_la_SOURCES = maxminddb_test_helper.c maxminddb_test_helper.h

EXTRA_DIST = compile_c++_t.pl decoder_limits_t.pl external_symbols_t.pl \
	     mmdblookup_t.pl maxmind-db

check_PROGRAMS = \
	bad_pointers_t bad_databases_t bad_data_size_t bad_epoch_t bad_indent_t \
	bad_search_tree_t \
	basic_lookup_t data_entry_list_t \
	data-pool-t data_types_t double_close_t dump_t empty_container_metadata_t \
	gai_error_t get_value_t \
	get_value_pointer_bug_t invalid_sockaddr_t \
	ipv4_start_cache_t ipv6_lookup_in_ipv4_t max_depth_t metadata_t \
	metadata_marker_t metadata_pointers_t no_map_get_value_t \
	overflow_bounds_t pointer_dos_t read_node_t \
	threads_t version_t

data_pool_t_LDFLAGS = $(AM_LDFLAGS) -lm
data_pool_t_SOURCES = data-pool-t.c ../src/data-pool.c

threads_t_CFLAGS = $(CFLAGS) -pthread

TESTS = $(check_PROGRAMS) compile_c++_t.pl decoder_limits_t.pl \
	external_symbols_t.pl mmdblookup_t.pl

LDADD = libmmdbtest.la $(CMOCKA_LIBS)
