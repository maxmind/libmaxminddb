#include "MMDB_Helper.h"

char *bytesdup(MMDB_return_s const *const ret)
{
    char *mem = NULL;
    if (ret->offset) {
        mem = malloc(ret->data_size + 1);
        memcpy(mem, ret->ptr, ret->data_size);
        mem[ret->data_size] = '\0';
    }
    return mem;
}

int addr_to_num(char *addr, struct in_addr *result)
{
    return inet_pton(AF_INET, addr, result);
}

int addr6_to_num(char *addr, struct in6_addr *result)
{
    return inet_pton(AF_INET6, addr, result);
}
