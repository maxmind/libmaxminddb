#include "MMDB.h"
#include "tap.h"

#ifndef MMDB_TEST_HELPER_C
#define MMDB_TEST_HELPER_C (1)

void for_all_modes( void (*tests)(int mode, const char* description) );

typedef union {
    struct in_addr v4;
    struct in6_addr v6;
} in_addrX;

void ip_string_to_struct(MMDB_s *mmdb, char *ipstr, in_addrX *dest_ipnum);

#endif
