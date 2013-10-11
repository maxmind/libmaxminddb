#!/usr/bin/env perl

use strict;
use warnings;

use File::Basename qw( basename );
use FindBin qw( $Bin );
use IPC::Run3;

my $top_dir = "$Bin/..";

my $output;

my @tests = map { ["$top_dir/t/.libs/$_"] } qw(
    lt-aget_value_t
    lt-basic_lookup_t
    lt-data_entry_list_t
    lt-data_types_t
    lt-dump_t
    lt-metadata_t
    lt-no_map_get_value_t
    lt-version_t
);

my @mmdblookup = (
    "$top_dir/bin/mmdblookup",
    '--file', "$top_dir/maxmind-db/test-data/MaxMind-DB-test-decoder.mmdb",
    '--ip',
);

# We want IPv4 and IPv6 addresses - one of each that exists in the db and one
# that doesn't
my @ips = ( '1.1.1.1', '10.0.0.0', 'abcd::', '0900::' );

my @cmds = (
    ( map { [ @mmdblookup, $_ ] } @ips ),
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
