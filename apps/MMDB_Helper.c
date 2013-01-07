#include "MMDB_Helper.h"

int addr_to_num(char *addr, struct in_addr *result)
{
    return inet_pton(AF_INET, addr, result);
}

int addr6_to_num(char *addr, struct in6_addr *result)
{
    return inet_pton(AF_INET6, addr, result);
}
