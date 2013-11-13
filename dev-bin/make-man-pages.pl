#!/usr/bin/env perl

use strict;
use warnings;
use autodie qw( :all );

use FindBin qw( $Bin );

use File::Path qw( mkpath );
use File::Slurp qw( read_file write_file );
use File::Temp qw( tempdir );
use File::Which qw( which );

sub main {
    my $target = shift || "$Bin/..";

    my $pandoc = which('pandoc')
        or die
        "\n  You must install pandoc in order to generate the man pages.\n\n";

    my $man_dir = "$target/man/man3";
    mkpath($man_dir);

    my $tempdir = tempdir( CLEANUP => 1 );

    my $markdown = <<'EOF';
% libmaxminddb(3)

EOF
    $markdown .=read_file("$Bin/../doc/libmaxminddb.md");

    my $tempfile = "$tempdir/libmaxminddb.3.md";
    write_file( $tempfile, $markdown );

    system(
        qw( pandoc -s -t man ), $tempfile, '-o',
        "$man_dir/libmaxminddb.3"
    );

    my $header = read_file("$Bin/../include/maxminddb.h");
    for my $proto ( $header =~ /^ +extern.+?(\w+)\(/gsm ) {
        open my $fh, '>', "$man_dir/$proto.3";
        print {$fh} ".so man3/libmaxminddb.3\n";
        close $fh;
    }
}

main(shift);
