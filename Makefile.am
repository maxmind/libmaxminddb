include_HEADERS = include/maxminddb.h include/maxminddb_config.h

SUBDIRS = \
  src     \
  bin     \
  t

EXTRA_DIST = doc
dist-hook:
	dev-bin/make-man-pages.pl $(distdir)

man3_MANS = man/man3/*.3

man/man3/*.3:
	true

