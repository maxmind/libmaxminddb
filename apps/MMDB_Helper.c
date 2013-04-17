#include "MMDB_Helper.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int is_ipv4(MMDB_s * mmdb)
{
    return mmdb->depth == 32;
}

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

void usage(char *prg)
{
    die("Usage: %s -f database addr\n", prg);
}

void dump_meta(MMDB_s * mmdb)
{
    MMDB_decode_all_s *decode_all = calloc(1, sizeof(MMDB_decode_all_s));
    int err = MMDB_get_tree(&mmdb->meta, &decode_all);
    assert(err == MMDB_SUCCESS);

    if (decode_all != NULL)
        MMDB_dump(decode_all, 0);
    free(decode_all);
}

void dump_ipinfo(const char * ipstr, MMDB_root_entry_s * ipinfo)
{

    double dlat, dlon;
    char *city, *country, *region_name, code2[3];
    if (ipinfo->entry.offset > 0) {
        MMDB_return_s res_location;
        MMDB_get_value(&ipinfo->entry, &res_location, "location", NULL);
        // TODO handle failed search somehow.
        MMDB_return_s lat, lon, field;
        MMDB_entry_s location = {.mmdb = ipinfo->entry.mmdb,.offset = res_location.offset
        };
        if (res_location.offset) {
            MMDB_get_value(&location, &lat, "latitude", NULL);
            MMDB_get_value(&location, &lon, "longitude", NULL);
            dlat = lat.double_value;
            dlon = lon.double_value;
        } else {
            dlon = dlat = 0;
        }

        //     printf( "%u %f %f\n", ipnum , dlat, dlon);

        MMDB_return_s res;
        MMDB_get_value(&ipinfo->entry, &res, "city", "names", "en", NULL);
        city = bytesdup(&res);
        MMDB_get_value(&ipinfo->entry, &res, "country", "names", "en", NULL);
        country = bytesdup(&res);

        printf("%s %f %f %s %s\n",ipstr, dlat, dlon, city == NULL ? "N/A" : city, country);

        if (city)
            free(city);
        if (country)
            free(country);

    } else {
        puts("Sorry, nothing found");   // not found
    }
}
