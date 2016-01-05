Current PPA process (should be added to release script):

1. git co ubuntu-ppa
2. git merge <TAG>
3. dch -i
  * Add new entry for wily (or whatever the most recent Ubuntu release
    is. Follow existing PPA versioning style.
4. git commit to add the debian changelog
5. gbp buildpackage -S
6. dput ppa:maxmind/ppa ../libmaxminddb_<TAG>-<DEB VERSION>_source.changes
7. git push

If 5 was successful, modify debian/changelog and repeat 5 & 6 for trusty and
precise. Note that you can skip step #4 for subsequent uploads by adding
`--git-ignore-new` when you call `gbp`.
