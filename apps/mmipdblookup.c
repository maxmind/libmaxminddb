#include "MMIPDB.h"
#include "MMIPDB_Helper.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
/* dummy content */

void usage(char *prg)
{
    fprintf(stderr, "Usage: %s -f database addr\n", prg);
    exit(1);
}

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

    //MMIPDB_s ipdb = MMIPDB_open( fname );
    if ( fname )
        free(fname);

    return (0);
}
