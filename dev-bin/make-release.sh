#!/bin/sh
set -e

TAG=$(perl -MFile::Slurp::Tiny=read_file -MDateTime <<EOF
use v5.16;
my \$today = DateTime->now->ymd;
my \$log = read_file(q{Changes.md});
\$log =~ /^\#\# (\d+\.\d+\.\d+(?:-\w+)?) - (\d{4}-\d{2}-\d{2})\n/;
die "Release time is not today! Release: \$2 Today: \$today"
    unless \$today eq \$2;
say \$1;
EOF
)

if [ -n "$(git status --porcelain)" ]; then
    echo ". is not clean." >&2
    exit 1
fi

old_version=$(perl -MFile::Slurp=read_file <<EOF
use v5.16;
my \$conf = read_file(q{configure.ac});
\$conf =~ /AC_INIT.+\[(\d+\.\d+\.\d+)\]/;
say \$1;
EOF
)

perl -MFile::Slurp=edit_file -e \
    "edit_file { s/\Q$old_version/$TAG/g } \$_ for qw( configure.ac include/maxminddb.h )"

if [ -n "$(git status --porcelain)" ]; then
    git add configure.ac include/maxminddb.h
    git commit -m "Bumped version to $TAG"
fi

if [ ! -d .gh-pages ]; then
    echo "Checking out gh-pages in .gh-pages"
    git clone -b gh-pages git@github.com:maxmind/libmaxminddb.git .gh-pages
    cd .gh-pages
else
    echo "Updating .gh-pages"
    cd .gh-pages
    git pull
fi

if [ -n "$(git status --porcelain)" ]; then
    echo ".gh-pages is not clean" >&2
    exit 1
fi

INDEX=index.md
cat <<EOF > $INDEX
---
layout: default
title: libmaxminddb - a library for working with MaxMind DB files
version: $TAG
---
EOF

cat ../doc/libmaxminddb.md >> $INDEX

MMDBLOOKUP=mmdblookup.md
cat <<EOF > $MMDBLOOKUP
---
layout: default
title: mmdblookup - a utility to look up an IP address in a MaxMind DB file
version: $TAG
---
EOF

cat ../doc/mmdblookup.md >> $MMDBLOOKUP

if [ -n "$(git status --porcelain)" ]; then
    git commit -m "Updated for $TAG" -a

    read -p "Push to origin? (y/n) " SHOULD_PUSH

    if [ "$SHOULD_PUSH" != "y" ]; then
        echo "Aborting"
        exit 1
    fi

    git push
fi

cd ..

git tag -a -m "Release for $TAG" "$TAG"
git push --follow-tags
