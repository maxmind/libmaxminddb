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
        fprintf(stderr, "You must provide a filename with -f");
        exit(1);
    }

    MMDB_s *mmdb = open_or_die(fname, MMDB_MODE_MMAP);

    free(fname);

    char *ipstr = argv[0];

    MMDB_lookup_result_s *result = lookup_or_die(mmdb, ipstr);

    if (verbose) {
        dump_meta(mmdb);
    }

    dump_ipinfo(ipstr, result);

    return (0);
}
