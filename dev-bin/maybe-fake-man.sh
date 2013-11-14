#!/bin/sh

if [ -f man/man3/libmaxminddb.3 ]; then
	true
else
	mkdir -p man/man3
	touch man/man3/libmaxminddb.3
fi
