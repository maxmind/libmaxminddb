#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "MMDB.h"
#include "MMDB_Helper.h"
#include "getopt.h"
#include <assert.h>

/* dummy content */

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
        fname = strdup("/usr/local/share/GeoIP2/city-v6.db");
    }

    assert(fname != NULL);

    MMDB_s *mmdb = MMDB_open(fname, MMDB_MODE_MEMORY_CACHE);

    if (!mmdb)
        die("Can't open %s\n", fname);

    free(fname);

    char *ipstr = argv[0];
    struct in6_addr ip;
    if (ipstr == NULL || 1 != addr6_to_num(ipstr, &ip))
        die("Invalid IP\n");

    if (verbose) {
      dump_meta(mmdb);
    }

    MMDB_root_entry_s root = {.entry.mmdb = mmdb };
    int err = MMDB_lookup_by_ipnum_128(ip, &root);
    if (err == MMDB_SUCCESS) {
        dump_ipinfo(ipstr, &root);
    }

    return (0);
}
