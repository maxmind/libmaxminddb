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

    MMDB_s mmdb = open_or_die(fname, MMDB_MODE_MMAP);

    free(fname);

    char *ipstr = argv[0];

    if (verbose) {
        dump_meta(mmdb);
    }

    MMDB_lookup_result_s *result = lookup_or_die(&mmdb, ipstr);

    if (result->entry.offset > 0) {
        MMDB_entry_data_list_s *entry_data_list;
        int status = MMDB_get_entry_data_list(&result->entry, &entry_data_list);
        if (MMDB_SUCCESS != status) {
            fprintf(stderr, "Got an error looking up the entry data - %s\n",
                    MMDB_strerror(status));
            exit(3);
        }
        MMDB_dump_entry_data_list(stdout, entry_data_list, 0);
    } else {
        puts("Sorry, nothing found");
    }

    return (0);
}
