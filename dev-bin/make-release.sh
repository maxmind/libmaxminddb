#!/bin/sh

VERSION=$@

git tag $VERSION
git push --tags
