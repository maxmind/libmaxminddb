#!/bin/sh

uncrustify="uncrustify -c .uncrustify.cfg --replace --no-backup"

# We indent each thing twice because ident is not idempotent - in some cases
# it will flip-flop between two indentation styles.
for dir in bin include src t; do
    c_files=`find $dir -maxdepth 1 -name '*.c'`
    if [ "$c_files" != "" ]; then
        $uncrustify $dir/*.c;
    fi
    
    h_files=`find $dir -maxdepth 1 -name '*.h'`
    if [ "$h_files" != "" ]; then
        $uncrustify $dir/*.h;
    fi
done

./dev-bin/regen-prototypes.pl
