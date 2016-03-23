# Releasing this library

We release by uploading the tarball to GitHub, uploading Ubuntu PPAs, and by
updating the Homebrew recipe for this library.

## Creating the release tarball

* Update `Changes.md` to include specify the new version, today's date, and
  list relevant changes.
* Run `./dev-bin/make-release.sh` to update various files in the distro, our
  GitHub pages, creates and pushes a tag, etc.
* Run `make safedist` in the checkout directory to create a tarball.

## GitHub

* Go to https://github.com/maxmind/libmaxminddb/releases
* GitHub will already have a release for the tag you just created.
* Click on the new tag new.
* Click on "Edit tag".
* Give the release a title summarizing the changes.
* Paste in the changes from the `Changes.md` file as the body.
* Attach the release tarball you just created.
* Click "Update release"

## PPA

In order to upload a PPA, you have to create a launchpad.net account and
register a GPG key with that account. You also need to be added to the MaxMind
team. Ask in the dev channel for someone to add you. See
https://help.launchpad.net/Packaging/PPA for more details.

The PPA release script is at `dev-bin/ppa-release.sh`. Running it should
guide you though the release, although it may require some changes to run on
configurations different than Greg's machine.

Pre-script PPA release process:

1. git co ubuntu-ppa
2. git merge `<TAG>`
3. dch -i
  * Add new entry for wily (or whatever the most recent Ubuntu release
    is. Follow existing PPA versioning style.
4. git commit to add the debian changelog
5. gbp buildpackage -S
6. dput ppa:maxmind/ppa ../libmaxminddb_`<TAG>`-`<DEB VERSION>`_source.changes
7. git push

If 5 was successful, modify debian/changelog and repeat 5 & 6 for trusty and
precise. Note that you can skip step #4 for subsequent uploads by adding
`--git-ignore-new` when you call `gbp`.

## Homebrew

* Go to https://github.com/Homebrew/homebrew/blob/master/Library/Formula/libmaxminddb.rb
* Edit the file to update the url and sha256. You can get the sha256 for the
  tarball with the `sha256sum` command line utility.
* Make a commit with the summary `libmaxminddb <VERSION>`
* Submit a PR with the changes you just made.
