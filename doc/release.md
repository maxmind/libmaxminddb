# How to Release this Library

Here's what you do:

## Update the package version in configure.ac

This is specified as the second argument to `AC_INIT`. This should be
something like 0.5.4 or 1.0.1.

## Run `make release` to create the git tag

This will create a tag based on the version in configure.ac and push it to the
default remote.

## Run `make dist` to create a tarball and upload it to GitHub

Go the [releases page](https://github.com/maxmind/libmaxminddb/releases) and
find the tag you just pushed. Then click "Edit tag".

You should change the title of the release to describe the highlights of the
release. Then copy the list of changes from Changes.md into main
textarea. Drag the tarball you just created to the "Attach binaries" spot on
the page. Click "Update release" and you're done.


