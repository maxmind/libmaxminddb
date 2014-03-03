## 0.5.4 - 2014-03-03

* Added support for compiling in the MinGW environment. Patch by Michael
  Eisendle.
* Added const declarations to many spots in the public API. None of these
  should require changes to existing code.
* Various documentation improvements.
* Changed the license to the Apache 2.0 license.


## 0.5.3 - 2013-12-23

* The internal value_for_key_as_uint16 method was returning a uint32_t instead
  of a uint16_t. Reported by Robert Wells. GitHub issue #11.
* The ip_version member of the MMDB_metadata_s struct was a uint8_t, even
  though the docs and spec said it should be a uint16_t. Reported by Robert
  Wells. GitHub issue #11.
* The mmdblookup_t.pl test now reports that it needs IPC::Run3 to run (which
  it always did, but it didn't tell you this). Patch by Elan Ruusamäe. GitHub
  issue #10.


## 0.5.2 - 2013-11-20

* Running `make` from the tarball failed. This is now fixed.


## 0.5.1 - 2013-11-20

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
