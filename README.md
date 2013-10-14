# About

The libmaxminddb library provides a C library for reading MaxMind DB files,
including the GeoIP2 databases from MaxMind. This is a custom binary format
designed to facilitate fast lookups of IP addresses while allowing for great
flexibility in the type of data associated with an address.

The MaxMind DB format is an open format. The spec is available at
http://maxmind.github.io/MaxMind-DB/. This spec is licensed under the Creative
Commons Attribution-ShareAlike 3.0 Unported License.

See http://dev.maxmind.com/ for more details about MaxMind's GeoIP2 products.

# License

This library is licensed under the GNU Lesser General Public License v. 2.1.

# Installing from a Tarball

This code is known to work with GCC 4.4+ and clang 3.2+. It should also work
on other compilers that supports C99, POSIX 2011.11, and the `-fms-extensions
flag` (or equivalent). The latter is needed to allow an anonymous union in a
structure.

To install this code, run the following commands:

    $ ./configure
    $ make
    $ make check
    $ make install

You can skip the `make check` step but it's always good to know that tests are
passing on your platform.

The `configure` script takes the standard options to set where files are
installed such as `--prefix`, etc. See `./configure --help` for details.

# Installing from the Git Repository

Our public git repository is hosted on GitHub at
https://github.com/maxmind/libmaxminddb

You can clone this repository and build it by running:

    $ git clone https://github.com/maxmind/libmaxminddb
    $ ./bootstrap
    $ ./configure
    $ make check

# Bug Reports

Please report bugs by filing an issue with our GitHub issue tracker at
https://github.com/maxmind/libmaxminddb/issues

# Dev Tools

We have a few development tools under the `dev-bin` directory to make
development easier. These are written in Perl or shell. They are:

* `regen-prototypes.pl` - This regenerates the prototypes in the header and
  source files. This helps keep headers and code in sync.
* `uncrustify-all.sh` - This runs `uncrustify` on all the code. It runs
  `regen-prototypes.pl` first. Please run this before submitting patches.
* `valgrind-all.pl` - This runs Valgrind on the tests and `mmdblookup` to
  check for memory leaks.

# Copyright and License

This software is Copyright (c) 2013 by MaxMind, Inc.

This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free
Software Foundation; either version 2.1 of the License, or (at your option)
any later version.

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
details.

You should have received a copy of the GNU Lesser General Public License along
with this library; if not, write to the Free Software Foundation, Inc., 51
Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
