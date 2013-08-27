#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "maxminddb.h"
#include "maxminddb_app_helper.h"
#include "getopt.h"
#include <assert.h>
#include <netdb.h>

int main(int argc, char *const argv[])
{
    int verbose = 0;
    int character;
    char *fname = NULL;
    MMDB_s *mmdb;
    uint16_t status;

    while ((character = getopt(argc, argv, "vf:")) != -1) {
        switch (character) {
        case 'v':
            verbose = 1;
            break;
        case 'f':
            fname = strdup(optarg);
            break;
        default:
        case '?':
            usage(argv[0]);
        }
    }
    argc -= optind;
    argv += optind;

    if (!fname) {
        fname = strdup(MMDB_DEFAULT_DATABASE);
    }

    assert(fname != NULL);

    status = MMDB_open(fname, MMDB_MODE_STANDARD, mmdb);
    if (!mmdb) {
        die("Can't open %s\n", fname);
    }

    free(fname);

    char *ipstr = argv[0];

    MMDB_lookup_result_s *result = lookup_or_die(mmdb, ipstr);

    if (verbose) {
        dump_meta(mmdb);
    }

    dump_ipinfo(ipstr, result);

    return (0);
}
