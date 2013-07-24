#include "MMDB.h"
#include "tap.h"

#ifndef MMDB_TEST_HELPER_C
#define MMDB_TEST_HELPER_C (1)

void for_all_modes( void (*tests)(int mode, const char* description) );

typedef union {
    struct in_addr v4;
    struct in6_addr v6;
} in_addrX;

MMDB_s *open_ok(char *db_file, int mode, char *mode_desc);

void snprintf_or_bail(char *target, size_t size, char *fmt, ...);

#endif
