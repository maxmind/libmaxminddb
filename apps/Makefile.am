AM_CPPFLAGS = -I$(top_srcdir)/libmaxminddb

AM_LDFLAGS = $(top_builddir)/libmaxminddb/libmaxminddb.la

lib_LTLIBRARIES = libmaxminddbapp.la
libmaxminddbapp_la_SOURCES = MMDB_Helper.c

bin_PROGRAMS = mmdblookup mmdbdump

LDADD = libmaxminddbapp.la
