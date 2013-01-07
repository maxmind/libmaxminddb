AM_CPPFLAGS =      \
        -I$(top_srcdir)/libmaxminddb

bin_PROGRAMS = mmdblookup

mmdblookup_SOURCES = mmdblookup.c MMDB_Helper.c

mmdblookup.lo mmdblookup.o: mmdblookup.c

MMDB_Helper.lo MMDB_Helper.o: MMDB_Helper.c

