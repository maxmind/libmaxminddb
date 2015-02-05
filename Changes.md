* Previously, when there was an error in `MMDB_open()`, `errno` would
  generally be overwritten during cleanup, preventing a useful value from
  being returned to the caller. This was changed so that the `errno` value
  from the function call that caused the error is restored before returning to
  the caller. In particular, this is important for `MMDB_IO_ERROR` errors as
  checking `errno` is often the only way to determine what actually failed.
* If `mmap()` fails due to running out of memory space, an
  `MMDB_OUT_OF_MEMORY_ERROR` is now returned from `MMDB_open` rather than an
  `MMDB_IO_ERROR`.
* On Windows, the `CreateFileMappingA()` handle was not properly closed if
  opening the database succeeded. Fixed by Bly Hostetler. GitHub #75 & #76.
* On Windows, we were not checking the return value of `CreateFileMappingA()`
  properly for errors. Fixed by Bly Hotetler. GitHub #78.


## 1.0.4 - 2014-01-02

* If you used a non-integer string as an array index when doing a lookup with
  `MMDB_get_value()`, `MMDB_vget_value()`, or `MMDB_aget_value()`, the first
  element of the array would be returned rather than an error. A
  `MMDB_LOOKUP_PATH_DOES_NOT_MATCH_DATA_ERROR` error will now be returned.
  GitHub #61.
* If a number larger than `LONG_MAX` was used in the same functions,
  `LONG_MAX` would have been used in the lookup. Now a
  `MMDB_INVALID_LOOKUP_PATH_ERROR` error will be returned.
* Visual Studio build files were added for unit tests and some compatibility
  issues with the tests were fixed.
* Visual Studio project was updated to use property pages. Patch by Andre.
  GitHub #69.
* A test failure in `t/compile_c++_t.pl` on new installs was fixed.


## 1.0.3 - 2014-12-02

* A memory and file handle leak on Win32 was fixed when getting the database
  size fails. Patch by Federico G. Schwindt. GitHub PR #49.
* Documentation fix. Federico G. Schwindt. GitHub PR #50.
* Added Visual Studio build files and fixed incorrect CreateFileMappingA
  usage. Patch by Andre. GitHub #52.
* The includes for the Windows header files were made lowercase in order to
  match the actual file names on case-sensitive file systems. GitHub PR #57.
* Removed `realloc()` calls that caused warnings on Windows and generally
  cleaned up memory allocation in `MMDB_vget_value()`. See relevant discussion
  in GitHub #52.
* Added an `extern "C" { ... }` wrapper to maxminddb.h when compiling with a
  C++ compiler. GitHub #55.


## 1.0.2 - 2014-09-22

* Fixed a number of small issues found by Coverity.
* When freeing the MMDB struct in `MMDB_close()` we make sure to set the
  pointers to NULL after freeing the memory they point to. This makes it safe
  to call `MMDB_close` more than once on the same `MMDB_s` struct
  pointer. Before this change, calling this function twice on the same pointer
  could cause the code to free memory that belonged to something else in the
  process. Patch by Shuxin Yang. GitHub PR #41.


## 1.0.1 - 2014-09-03

* Added missing LICENSE and NOTICE files to distribution. No code changes.


## 1.0.0 - 2014-09-02

* Bumped version to 1.0.0. No code changes.


## 0.5.6 - 2014-07-21

* There was a leak in the `MMDB_open()` sub when it was called against a file
  which did not contain any MMDB metadata. Reported by Federico
  G. Schwindt. GitHub issue #36.
* Fixed an error that occurred when passing AI_V4MAPPED to `getaddrinfo()` on
  FreeBSD. Apparently this macro is defined but doesn't work the way we
  expected it to on that platform.
* Made sure to call `freeaddrinfo()` when a call to `getaddrinfo()` fails but
  still allocated memory.
* Fixed a segfault in the tests that occurred on FreeBSD if we passed a NULL
  value to `freeaddrinfo()`.
* Added a missing step to the README.md file for installing from our GitHub
  repository. Patch by Yasith Fernando.
* Added instructions for installing via Homebrew. Patch by Yasith Fernando.


## 0.5.5 - 2014-03-11

* The previous tarball failed to compile because it was missing the
  src/maxminddb-compat-util.h file. Reported by Günter Grodotzki. GitHub issue
  #18.


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
