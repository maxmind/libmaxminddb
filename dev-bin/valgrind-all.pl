#!/usr/bin/env perl

use strict;
use warnings;

use File::Basename qw( basename );
use FindBin qw( $Bin );
use IPC::Run3;

my $top_dir = "$Bin/..";

my $output;

my @tests = map { ["$top_dir/t/.libs/$_"] } qw(
    lt-basic_lookup_t
    lt-data_entry_list_t
    lt-data_types_t
    lt-metadata_t
    lt-no_map_get_value_t
    lt-version_t
);

my @cmds = (
    [
        "$top_dir/bin/mmdblookup",
        '-f', "$top_dir/maxmind-db/test-data/MaxMind-DB-test-decoder.mmdb",
        '1.1.1.1'
    ],
    [
        "$top_dir/bin/mmdbdump",
        '-f', "$top_dir/maxmind-db/test-data/MaxMind-DB-test-decoder.mmdb",
        '1.1.1.1'
    ],
    @tests,
);

for my $cmd (@cmds) {
    my $output;
    run3(
        [ qw( valgrind --leak-check=full -- ), @{$cmd} ],
        \undef,
        \$output,
        \$output,
    );

    $output =~ s/^(?!=).*\n//mg;

    my $marker = '-' x 60;
    print $marker, "\n", ( join q{ }, basename( shift @{$cmd} ), @{$cmd} ),
        "\n", $marker, "\n", $output,
        "\n\n";
}
