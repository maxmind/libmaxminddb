AM_CPPFLAGS = -I$(top_srcdir)/include

AM_LDFLAGS = $(top_builddir)/src/libmaxminddb.la

lib_LTLIBRARIES = libmaxminddbapp.la
libmaxminddbapp_la_SOURCES = maxminddb_app_helper.c

bin_PROGRAMS = mmdblookup mmdbdump

LDADD = libmaxminddbapp.la
