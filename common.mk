if DEBUG
AM_CFLAGS=-O0 -g -Wall -Wextra --std=c99
else
AM_CFLAGS=-O2 -g --std=c99
endif

AM_CPPFLAGS = -I$(top_srcdir)/include
