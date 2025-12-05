#!/bin/bash

set -eu -o pipefail

# Pre-flight checks - verify all required tools are available and configured
# before making any changes to the repository

check_command() {
    if ! command -v "$1" &>/dev/null; then
        echo "Error: $1 is not installed or not in PATH"
        exit 1
    fi
}

# Verify gh CLI is authenticated
if ! gh auth status &>/dev/null; then
    echo "Error: gh CLI is not authenticated. Run 'gh auth login' first."
    exit 1
fi

# Verify we can access this repository via gh
if ! gh repo view --json name &>/dev/null; then
    echo "Error: Cannot access repository via gh. Check your authentication and repository access."
    exit 1
fi

# Verify git can connect to the remote (catches SSH key issues, etc.)
if ! git ls-remote origin &>/dev/null; then
    echo "Error: Cannot connect to git remote. Check your git credentials/SSH keys."
    exit 1
fi

check_command perl
check_command make
check_command autoconf

# Check that we're not on the main branch
current_branch=$(git branch --show-current)
if [ "$current_branch" = "main" ]; then
    echo "Error: Releases should not be done directly on the main branch."
    echo "Please create a release branch and run this script from there."
    exit 1
fi

# Fetch latest changes and check that we're not behind origin/main
echo "Fetching from origin..."
git fetch origin

if ! git merge-base --is-ancestor origin/main HEAD; then
    echo "Error: Current branch is behind origin/main."
    echo "Please merge or rebase with origin/main before releasing."
    exit 1
fi

changelog=$(cat Changes.md)

regex='## ([0-9]+\.[0-9]+\.[0-9]+(-[a-zA-Z0-9.]+)?) - ([0-9]{4}-[0-9]{2}-[0-9]{2})

((.|
)*)
'

if [[ ! $changelog =~ $regex ]]; then
    echo "Could not find date line in change log!"
    exit 1
fi

version="${BASH_REMATCH[1]}"
date="${BASH_REMATCH[3]}"
notes="$(echo "${BASH_REMATCH[4]}" | sed -n -E '/^## [0-9]+\.[0-9]+\.[0-9]+(-[a-zA-Z0-9.]+)?/,$!p')"

dist="libmaxminddb-$version.tar.gz"

if [[ "$date" != "$(date +"%Y-%m-%d")" ]]; then
    echo "$date is not today!"
    exit 1
fi

if [ -n "$(git status --porcelain)" ]; then
    echo ". is not clean." >&2
    exit 1
fi

old_version=$(
    perl -MFile::Slurp=read_file <<EOF
use v5.16;
my \$conf = read_file(q{configure.ac});
\$conf =~ /AC_INIT.+\[(\d+\.\d+\.\d+)\]/;
say \$1;
EOF
)

perl -MFile::Slurp=edit_file -e \
    "edit_file { s/\Q$old_version/$version/g } \$_ for qw( configure.ac include/maxminddb.h CMakeLists.txt )"

if [ -n "$(git status --porcelain)" ]; then
    git diff

    read -r -e -p "Commit changes? (y/n) " should_commit

    if [ "$should_commit" != "y" ]; then
        echo "Aborting"
        exit 1
    fi

    git add configure.ac include/maxminddb.h CMakeLists.txt
    git commit -m "Bumped version to $version"
fi

./bootstrap
./configure
make
make check
make clean
make safedist

if [ ! -d .gh-pages ]; then
    echo "Checking out gh-pages in .gh-pages"
    git clone -b gh-pages git@github.com:maxmind/libmaxminddb.git .gh-pages
    pushd .gh-pages
else
    echo "Updating .gh-pages"
    pushd .gh-pages
    git pull
fi

if [ -n "$(git status --porcelain)" ]; then
    echo ".gh-pages is not clean" >&2
    exit 1
fi

index=index.md
cat <<EOF >$index
---
layout: default
title: libmaxminddb - a library for working with MaxMind DB files
version: $version
---
EOF

cat ../doc/libmaxminddb.md >>$index

mmdblookup=mmdblookup.md
cat <<EOF >$mmdblookup
---
layout: default
title: mmdblookup - a utility to look up an IP address in a MaxMind DB file
version: $version
---
EOF

cat ../doc/mmdblookup.md >>$mmdblookup

git commit -m "Updated for $version" -a

read -r -e -p "Push to origin? (y/n) " should_push

if [ "$should_push" != "y" ]; then
    echo "Aborting"
    exit 1
fi

git push

popd

git push

gh release create --target "$(git branch --show-current)" -t "$version" -n "$notes" "$version" "$dist"
