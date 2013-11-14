include_HEADERS = include/maxminddb.h include/maxminddb_config.h

SUBDIRS = \
  src     \
  bin     \
  t

EXTRA_DIST = doc maxmind-db t
dist-hook:
	dev-bin/make-man-pages.pl $(distdir)
	find $(distdir) -name '.git*' | xargs rm -fr

man3_MANS = man/man3/*.3

man/man3/*.3:
	true

.PHONY: man/man3/*.3
