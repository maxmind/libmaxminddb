# Releasing this library

We release by uploading the tarball to GitHub, uploading Ubuntu PPAs, and by
updating the Homebrew recipe for this library.

## Creating the release tarball

* Update `Changes.md` to include specify the new version, today's date, and
  list relevant changes.
* Run `./dev-bin/release.sh` to update various files in the distro, our
  GitHub pages, and creates a GitHub release with the tarball.

## PPA

In order to upload a PPA, you have to create a launchpad.net account and
register a GPG key with that account. You also need to be added to the MaxMind
team. Ask in the dev channel for someone to add you. See
https://help.launchpad.net/Packaging/PPA for more details.

The PPA release script is at `dev-bin/ppa-release.sh`. Running it should
guide you though the release, although it may require some changes to run on
configurations different than Greg's machine.

## Homebrew

* Go to https://github.com/Homebrew/homebrew/blob/master/Library/Formula/libmaxminddb.rb
* Edit the file to update the url and sha256. You can get the sha256 for the
  tarball with the `sha256sum` command line utility.
* Make a commit with the summary `libmaxminddb <VERSION>`
* Submit a PR with the changes you just made.
