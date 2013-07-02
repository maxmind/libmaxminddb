
#ifndef TEST_HELPER_C
#define TEST_HELPER_C (1)

#include "MMDB.h"

typedef union {
    struct in_addr v4;
    struct in6_addr v6;
} in_addrX;

char *get_test_db_fname(void);
void ip_to_num(MMDB_s * mmdb, char *ipstr, in_addrX * dest_ipnum);
int dbl_cmp(double a, double b);
#endif
