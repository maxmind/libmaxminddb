AM_CPPFLAGS = -I$(top_srcdir)/include

AM_LDFLAGS = $(top_builddir)/src/libmaxminddb.la

lib_LTLIBRARIES = libmaxminddbapp.la
libmaxminddbapp_la_SOURCES = MMDB_Helper.c

bin_PROGRAMS = mmdblookup mmdbdump

LDADD = libmaxminddbapp.la
