#include "MMDB_Helper.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int is_ipv4(MMDB_s * mmdb)
{
    return mmdb->depth == 32;
}

char *bytesdup(MMDB_s * mmdb, MMDB_return_s const *const ret)
{
    char *mem = NULL;

    if (ret->offset) {
        mem = malloc(ret->data_size + 1);

        if (mmdb && mmdb->fd >= 0) {
            uint32_t segments = mmdb->full_record_size_bytes * mmdb->node_count;
            MMDB_pread(mmdb->fd, (void *)mem, ret->data_size,
                       segments + (uintptr_t) ret->ptr);
        } else {
            memcpy(mem, ret->ptr, ret->data_size);
        }
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
        MMDB_dump(NULL, decode_all, 0);
    free(decode_all);
}

static const char *na(char const *string)
{
    return string ? string : "N/A";
}

void dump_ipinfo(const char *ipstr, MMDB_root_entry_s * ipinfo)
{

    char *city, *country, *region;
    double dlat, dlon;
    dlat = dlon = 0;
    if (ipinfo->entry.offset > 0) {
        MMDB_return_s res_location;
        MMDB_get_value(&ipinfo->entry, &res_location, "location", NULL);
        // TODO handle failed search somehow.
        MMDB_return_s lat, lon;
        MMDB_entry_s location = {.mmdb = ipinfo->entry.mmdb,.offset =
                res_location.offset
        };
        if (res_location.offset) {
            MMDB_get_value(&location, &lat, "latitude", NULL);
            MMDB_get_value(&location, &lon, "longitude", NULL);
            if (lat.offset)
                dlat = lat.double_value;
            if (lon.offset)
                dlon = lon.double_value;
        }

        MMDB_return_s res;
        MMDB_get_value(&ipinfo->entry, &res, "city", "names", "en", NULL);
        city = bytesdup(ipinfo->entry.mmdb, &res);
        MMDB_get_value(&ipinfo->entry, &res, "country", "names", "en", NULL);
        country = bytesdup(ipinfo->entry.mmdb, &res);

        MMDB_get_value(&ipinfo->entry, &res, "subdivisions", "0", "names", "en", NULL);
        region = bytesdup(ipinfo->entry.mmdb, &res);

        printf("%s %f %f %s %s %s\n", ipstr, dlat, dlon,
	       na(region),na(city), na(country));
        free_list(city, country, region);
    } else {
        puts("Sorry, nothing found");   // not found
    }
}
