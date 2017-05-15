#!/bin/bash

set -e
set -x
set -u

DISTS=( wily trusty precise )

VERSION=$(perl -MFile::Slurp::Tiny=read_file -MDateTime <<EOF
use v5.16;
my \$log = read_file(q{Changes.md});
\$log =~ /^## (\d+\.\d+\.\d+) - (\d{4}-\d{2}-\d{2})/;
die 'Release time is not today!' unless DateTime->now->ymd eq \$2;
say \$1;
EOF
)

RESULTS=/tmp/build-libmaxminddb-results/
SRCDIR="$RESULTS/libmaxminddb"

mkdir -p "$SRCDIR"

# gbp does weird things without a pristine checkout
git clone git@github.com:maxmind/libmaxminddb.git -b ubuntu-ppa $SRCDIR

pushd "$SRCDIR"
git merge "$VERSION"

for dist in "${DISTS[@]}"; do
    git clean -xfd
    git reset HEAD --hard

    dch -v "$VERSION-0+maxmind1~$dist" -D "$dist" -u low "New upstream release."
    gbp buildpackage -S --git-ignore-new
done

read -e -p "Release to PPA? (y/n)" SHOULD_RELEASE

if [ "$SHOULD_RELEASE" != "y" ]; then
    echo "Aborting"
    exit 1
fi

dput ppa:maxmind/ppa ../*source.changes

popd

git add debian/changelog
git commit -m "Update debian/changelog for $VERSION"
git push
