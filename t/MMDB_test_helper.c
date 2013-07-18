#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "MMDB.h"
#include "MMDB_test_helper.h"

void for_all_modes( void (*tests)(int mode, const char *description) )
{
    tests(MMDB_MODE_STANDARD, "standard mode");
    tests(MMDB_MODE_MEMORY_CACHE, "memory cache mode");
}

void ip_string_to_struct(MMDB_s *mmdb, char *ipstr, in_addrX *dest_ipnum)
{
    int ai_family = mmdb->depth == 32 ? AF_INET : AF_INET6;
    int ai_flags = AI_V4MAPPED;

    if (ipstr == NULL || 0 != MMDB_resolve_address(ipstr, ai_family, ai_flags,
                                                  dest_ipnum)) {
        fprintf(stderr, "Invalid IP\n");
        exit(1);
    }
}
