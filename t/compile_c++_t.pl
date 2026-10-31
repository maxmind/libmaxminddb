#!/usr/bin/env perl

use strict;
use warnings;

use Cwd qw( abs_path );
use FindBin qw( $Bin );

eval <<'EOF';
use Test::More 0.88;
use File::Temp qw( tempdir );
use IPC::Run3 qw( run3 );
EOF

if ($@) {
    print
        "1..0 # skip all tests skipped - these tests need the Test::More 0.88, File::Temp, and IPC::Run3 modules:\n";
    print "$@";
    exit 0;
}

my $test_db = "$Bin/maxmind-db/test-data/GeoIP2-City-Test.mmdb";

my $cpp_code = <<"EOF";
#include <maxminddb.h>

int main()
{
    const char *fname = "$test_db";
    MMDB_s mmdb;
    if (MMDB_open(fname, MMDB_MODE_MMAP, &mmdb) != MMDB_SUCCESS) {
        return 1;
    }

    MMDB_lookup_context_s *context = NULL;
    int status = MMDB_create_lookup_context(&mmdb, &context);
    if (status == MMDB_SUCCESS) {
        struct sockaddr_in address = {};
        address.sin_family = AF_INET;
        int gai_error, socket_error;
        MMDB_lookup_sockaddr_with_context(
            context, reinterpret_cast<const struct sockaddr *>(&address),
            &socket_error);
        MMDB_lookup_string_with_context(
            context, "81.2.69.160", &gai_error, &status);
        status = status || gai_error || socket_error;
    }

    MMDB_free_lookup_context(context);
    MMDB_close(&mmdb);
    return status;
}
EOF

my $tempdir = tempdir(CLEANUP => 1 );

my $file = "$tempdir/open.cpp";
open my $fh, '>', $file or die $!;
print {$fh} $cpp_code or die $!;
close $fh or die $!;

my $exe = "$tempdir/open";

my $include_dir = abs_path("$Bin/../include");
my $lib_dir = abs_path("$Bin/../src/.libs");

my $cxx = $ENV{CXX} || 'c++';
my @cxxflags = $ENV{CXXFLAGS} ? ( split ' ', $ENV{CXXFLAGS} ) : ();
_test_cmd(
    [
        $cxx,
        $file,
        @cxxflags,
        "-I$include_dir",
        "-L$lib_dir",
        "-lmaxminddb",
        "-o$exe",
    ],
    qr/^$/,
    q{},
    0,
    'compile C++ program which links against libmaxminddb',
);

# DYLD_LIBRARY_PATH is for Mac OS X
$ENV{LD_LIBRARY_PATH} = $ENV{DYLD_LIBRARY_PATH} = $lib_dir;

_test_cmd(
    [$exe],
    qr/^$/,
    q{},
    0,
    'compiled C++ program executes without errors'
);

done_testing();

sub _test_cmd {
    my $cmd          = shift;
    my $expect_stdout = shift;
    my $expect_stderr = shift;
    my $expect_status = shift;
    my $desc          = shift;

    my $stdout;
    my $stderr;
    run3(
        $cmd,
        \undef,
        \$stdout,
        \$stderr,
    );

    my $exit_status = $? >> 8;

    # We don't need to retest that the help output shows up for all errors
    if ( defined $expect_stdout ) {
        like(
            $stdout,
            $expect_stdout,
            "stdout for @{$cmd}"
        );
    }

    if ( ref $expect_stderr ) {
        like( $stderr, $expect_stderr, "stderr for @{$cmd}" );
    }
    else {
        is( $stderr, $expect_stderr, "stderr for @{$cmd}" );
    }

    is(
        $exit_status, $expect_status,
        "exit status was $expect_status for @{$cmd}"
    );
}
