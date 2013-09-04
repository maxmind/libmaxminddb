#include "maxminddb_app_helper.h"
#include <assert.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>

char *bytesdup(MMDB_s * mmdb, MMDB_entry_data_s const *const entry_data)
{
    char *mem = NULL;

    if (entry_data->offset) {
        mem = malloc(entry_data->data_size + 1);

        memcpy(mem, entry_data->utf8_string, entry_data->data_size);
        mem[entry_data->data_size] = '\0';
    }
    return mem;
}

void usage(char *prg)
{
    die("Usage: %s -f database addr\n", prg);
}

// XXX - this should use the metadata struct, not re-decode the metadata
void dump_meta(MMDB_s mmdb)
{
    // XXX - temporarily a no-op until I can make it use the metadata struct
    return;
}

static const char *na(char const *string)
{
    return string ? string : "N/A";
}

MMDB_s open_or_die (const char *fname, int mode)
{
    MMDB_s mmdb;
    int status = MMDB_open(fname, MMDB_MODE_MMAP, &mmdb);

    if (MMDB_SUCCESS != status) {
        fprintf(stderr, "Can't open %s\n", fname);
        exit(2);
    }

    return mmdb;
}

MMDB_lookup_result_s *lookup_or_die (MMDB_s *mmdb, const char *ipstr)
{
    int gai_error, mmdb_error;
    MMDB_lookup_result_s *result =
        MMDB_lookup(mmdb, ipstr, &gai_error, &mmdb_error);

    if (0 != gai_error) {
        fprintf(stderr, "error from call to getaddrinfo for %s - %s\n", ipstr,
                gai_strerror(gai_error));
        exit(1);
    }

    if (MMDB_SUCCESS != mmdb_error) {
        fprintf(stderr, "got an error from the maxminddb library: %s\n", MMDB_strerror(mmdb_error));
        exit(2);
    }

    return result;
}

void dump_ipinfo(const char *ipstr, MMDB_lookup_result_s * ipinfo)
{

    char *city, *country, *region;
    double dlat, dlon;
    dlat = dlon = 0;
    if (ipinfo->entry.offset > 0) {
        MMDB_entry_data_s entry_data;
        MMDB_get_value(&ipinfo->entry, &entry_data, "location", NULL);
        // TODO handle failed search somehow.
        MMDB_entry_data_s lat, lon;
        MMDB_entry_s location = {.mmdb = ipinfo->entry.mmdb,.offset =
                entry_data.offset
        };
        if (entry_data.offset) {
            MMDB_get_value(&location, &lat, "latitude", NULL);
            MMDB_get_value(&location, &lon, "longitude", NULL);
            if (lat.offset)
                dlat = lat.double_value;
            if (lon.offset)
                dlon = lon.double_value;
        }

        MMDB_entry_data_s res;
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
