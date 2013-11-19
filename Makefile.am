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
	dev-bin/maybe-fake-man.sh

release:
	dev-bin/make-release.sh $(PACKAGE_VERSION)

.PHONY: man/man3/*.3 release
