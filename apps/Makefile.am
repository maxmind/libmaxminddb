AM_CPPFLAGS =      \
        -I$(top_srcdir)/libmaxminddb

bin_PROGRAMS = mmdblookup6 mmdblookup mmdbdump country_lookup

mmdblookup_DEPENDENCIES = $(top_builddir)/libmaxminddb/libmaxminddb.la
mmdblookup_LDADD = $(top_builddir)/libmaxminddb/libmaxminddb.la -lc
mmdblookup_SOURCES = mmdblookup.c MMDB_Helper.c

mmdblookup.lo mmdblookup.o: mmdblookup.c

mmdblookup6_DEPENDENCIES = $(top_builddir)/libmaxminddb/libmaxminddb.la
mmdblookup6_LDADD = $(top_builddir)/libmaxminddb/libmaxminddb.la -lc
mmdblookup6_SOURCES = mmdblookup6.c MMDB_Helper.c

mmdblookup6.lo mmdblookup6.o: mmdblookup6.c


mmdbdump_DEPENDENCIES = $(top_builddir)/libmaxminddb/libmaxminddb.la
mmdbdump_LDADD = $(top_builddir)/libmaxminddb/libmaxminddb.la -lc
mmdbdump_SOURCES = mmdbdump.c MMDB_Helper.c
mmdbdump.lo mmdbdump.o: mmdbdump.c

country_lookup_DEPENDENCIES = $(top_builddir)/libmaxminddb/libmaxminddb.la
country_lookup_LDADD = $(top_builddir)/libmaxminddb/libmaxminddb.la -lc
country_lookup_SOURCES = country_lookup.c MMDB_Helper.c
coutry_lookup.lo country_lookup.o: country_lookup.c

MMDB_Helper.lo MMDB_Helper.o: MMDB_Helper.c

