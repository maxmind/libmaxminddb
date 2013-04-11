#include <stdlib.h>
#include "MMDB.h"
#include <netdb.h>
#include <stdio.h>
#include "test_helper.h"

char *get_test_db_fname(void)
{
    char *fname = getenv("MMDB_TEST_DATABASE");
    if (!fname)
        fname = MMDB_DEFAULT_DATABASE;
    return fname;
}

void ip_to_num(MMDB_s * mmdb, char *ipstr, in_addrX * dest_ipnum)
{
    int ai_family = mmdb->depth == 32 ? AF_INET : AF_INET6;
    int ai_flags = AI_V4MAPPED; // accept every crap

    if (ipstr == NULL || 0 != MMDB_lookupaddressX(ipstr, ai_family, ai_flags,
                                                  dest_ipnum)) {
        fprintf(stderr, "Invalid IP\n");
        exit(1);
    }
}
