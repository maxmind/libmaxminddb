include_HEADERS = include/maxminddb.h

include_execdir = $(exec_prefix)/include

nodist_include_exec_HEADERS = include/maxminddb_config.h

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
	if [ ! -f man/man3/libmaxminddb.3 ]; then mkdir -p man/man3 && touch man/man3/libmaxminddb.3; fi

release:
	dev-bin/make-release.sh $(PACKAGE_VERSION)

.PHONY: man/man3/*.3 release
