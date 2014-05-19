#!/usr/bin/env perl

use strict;
use warnings;
use autodie qw( :all );

use FindBin qw( $Bin );

use File::Path qw( mkpath );
use File::Slurp qw( edit_file read_file write_file );
use File::Temp qw( tempdir );
use File::Which qw( which );

sub main {
    my $target = shift || "$Bin/..";

    my $pandoc = which('pandoc')
        or die
        "\n  You must install pandoc in order to generate the man pages.\n\n";

    _make_man( $target, 'libmaxminddb', 3 );
    _make_lib_man_links($target);

    _make_man( $target, 'mmdblookup', 1 );
}

sub _make_man {
    my $target  = shift;
    my $name    = shift;
    my $section = shift;

    my $man_dir = "$target/man/man$section";
    mkpath($man_dir);

    my $tempdir = tempdir( CLEANUP => 1 );

    my $markdown = <<"EOF";
% $name($section)

EOF
    $markdown .= read_file("$Bin/../doc/$name.md");

    my $tempfile = "$tempdir/$name.$section.md";
    write_file( $tempfile, $markdown );

    my $man_file = "$man_dir/$name.$section";
    system( qw( pandoc -s -t man ), $tempfile, '-o', $man_file );

    _fix_indentation($man_file);
}

sub _make_lib_man_links {
    my $target = shift;

    my $header = read_file("$Bin/../include/maxminddb.h");
    for my $proto ( $header =~ /^ +extern.+?(\w+)\(/gsm ) {
        open my $fh, '>', "$target/man/man3/$proto.3";
        print {$fh} ".so man3/libmaxminddb.3\n";
        close $fh;
    }
}

# AFAICT there's no way to control the indentation depth for code blocks with
# Pandoc.
sub _fix_indentation {
    my $file = shift;

    edit_file(
        sub {
            s/^\.IP\n\.nf/.IP "" 4\n.nf/gm;
        },
        $file
    );
}

main(shift);
