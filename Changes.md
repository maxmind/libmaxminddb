## 0.5.1

* Renamed MMDB_LOOKUP_PATH_DOES_NOT_MATCH_DATA define to
  MMDB_LOOKUP_PATH_DOES_NOT_MATCH_DATA_ERROR for consistency. Fixes github
  issue #5. Reported by Albert Strasheim.
* Updated README.md to show git clone with --recursive flag so you get the
  needed submodules. Fixes github issue #4. Reported by Ryan Peck.
* Fixed some bugs with the MMDB_get_*value functions when navigating a data
  structure that included pointers. Fixes github issue #3. Reported by
  bagadon.
* Fixed compilation problems on OSX and OpenBSD. We have tested this on OSX
  and OpenBSD 5.4. Fixes github issue #6.
* Removed some unneeded memory allocations and added const to many variable
  declarations. Based on patches by Timo Teräs. Github issue #8.
* Added a test that uses threads to check for thread safety issue in the
  library.
* Distro tarball now includes man pages, tests, and test data
