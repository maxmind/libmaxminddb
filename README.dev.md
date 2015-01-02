Current PPA process (should be added to release script):

1. git co ubuntu-ppa
2. git merge <TAG>
3. dch -i
  * Add new entry for utopic. Follow existing PPA versioning style.
4. gbp buildpackage -S
5. dput ppa:maxmind/ppa ../libmaxminddb_<TAG>-<DEB VERSION>_source.changes

If 5 was successful, modify debian/changelog and repeate 4 & 5 for trusty and
precise.
