include_HEADERS = include/maxminddb.h include/maxminddb_config.h

SUBDIRS = \
  src     \
  bin     \
  t

EXTRA_DIST = doc
dist-hook:
	dev-bin/make-man-pages.pl $(distdir)

man_MANS = man/man3/*.3
