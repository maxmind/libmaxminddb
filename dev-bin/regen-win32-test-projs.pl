#!/usr/bin/env perl

use strict;
use warnings;

use FindBin qw( $Bin );

use Data::UUID;
use File::Slurp qw( read_file write_file );
use Path::Iterator::Rule;

sub main {
    my $rule = Path::Iterator::Rule->new;
    $rule->file->name(qr/_t.c$/);

    my $ug = Data::UUID->new;

    my $template = read_file("$Bin/../projects/test.vcxproj.template");

    for my $file ( $rule->all("$Bin/../t/") ) {
        my ($name) = $file =~ /(\w*)_t.c$/;

        next unless $name;

        next if $name eq 'threads';
        my $project = $template;

        $project =~ s/%TESTNAME%/$name/g;

        my $uuid = $ug->to_string( $ug->create );
        $project =~ s/%UUID%/$uuid/g;

        write_file( "$Bin/../projects/VS12-tests/$name.vcxproj", $project );
    }
}

main();
