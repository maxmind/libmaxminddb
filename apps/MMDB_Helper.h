#ifndef GEODB_HELPER
#define GEODB_HELPER (1)
#include "MMDB.h"
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

int addr_to_num(char *addr, struct in_addr *result);
int addr6_to_num(char *addr, struct in6_addr *result);
char *bytesdup(MMDB_return_s const *const ret);
#endif
