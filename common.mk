if DEBUG
AM_CFLAGS=-O0 -g -Wall -Wextra -lm
else
AM_CFLAGS=-O2 -g -lm
endif

AM_CPPFLAGS = -I$(top_srcdir)/include
