#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "MMDB.h"
#include "MMDB_Helper.h"
#include "getopt.h"

// country lookup is not useful, it is just to see how fast it is compared to
// 106

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

    MMDB_s *mmdb =
        MMDB_open(fname ? fname :
                  MMDB_DEFAULT_DATABASE,
                  MMDB_MODE_MEMORY_CACHE);

    for (int i = 1; i <= 10000000; i++) {

        MMDB_root_entry_s root = {.entry.mmdb = mmdb };
//    int err = MMDB_lookup_by_ipnum(404232216, &root);
        uint32_t ipnum = htonl(rand());
        int err = MMDB_lookup_by_ipnum(ipnum, &root);
        if (err == MMDB_SUCCESS) {
            double dlat, dlon;
            char *name, *code;
            if (root.entry.offset > 0) {
                MMDB_return_s country;
                MMDB_get_value(&root.entry, &country, "country", NULL);
                MMDB_entry_s start = {.mmdb = mmdb,.offset = country.offset };
                if (country.offset) {
                    MMDB_return_s res;
                    MMDB_get_value(&start, &res, "code", NULL);
                    code = bytesdup(&res);
                    MMDB_get_value(&start, &res, "name", "en", NULL);
                    name = bytesdup(&res);

//                    printf( "%u %s %s\n", ipnum , code, name);

                    if (name)
                        free(name);
                    if (code)
                        free(code);
                }
            } else {
                ;               // not found
            }
        }
    }

    if (fname)
        free(fname);

    return (0);
}
