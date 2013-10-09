# This is really gross since we end up with "--std=gnu99 ... --std=c99" in the
# flags. I can't figure out how to tell autoconf to _not_ use gnu99.

if DEBUG
AM_CFLAGS=-O0 -g -Wall -Wextra --std=c99
else
AM_CFLAGS=-O2 -g --std=c99
endif

AM_CPPFLAGS = -I$(top_srcdir)/include
