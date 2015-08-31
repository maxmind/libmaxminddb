include $(top_srcdir)/common.mk

lib_LTLIBRARIES = libmaxminddb.la

libmaxminddb_la_SOURCES = maxminddb.c maxminddb-compat-util.h
libmaxminddb_la_LDFLAGS = -version-info 0:7:0
include_HEADERS = $(top_srcdir)/include/maxminddb.h

pkgconfig_DATA = libmaxminddb.pc
