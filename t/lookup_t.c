#include "MMDB.h"
#include "tap.h"
#include <sys/stat.h>
#include <arpa/inet.h>
#include <string.h>
#include "test_helper.h"

uint32_t ip_to_num(char *ipstr)
{
    struct in_addr ip;
    if (ipstr == NULL || 1 != inet_pton(AF_INET, ipstr, &ip))
        return 0;
    return htonl(ip.s_addr);
}

void test_mmdb(MMDB_s * mmdb)
{

    MMDB_root_entry_s root = {.entry.mmdb = mmdb };
    char *ipstr = "127.0.0.1";
    uint32_t ipnum = ip_to_num(ipstr);
    int err = MMDB_lookup_by_ipnum(ipnum, &root);
    ok(err == MMDB_SUCCESS, "Search for %s SUCCESSFUL", ipstr);
    ok(root.entry.offset == 0, "No entries found for %s good", ipstr);
    ipstr = "24.24.24.24";
    ipnum = ip_to_num(ipstr);
    err = MMDB_lookup_by_ipnum(ipnum, &root);
    ok(err == MMDB_SUCCESS, "Search for %s SUCCESSFUL", ipstr);
    ok(root.entry.offset > 0, "Found something %s good", ipstr);
    MMDB_return_s country;
    MMDB_get_value(&root.entry, &country, "country", NULL);
    ok(country.offset > 0, "Found country hash for %s", ipstr);
    if (country.offset) {
        MMDB_entry_s country_hash = {.mmdb = mmdb,.offset = country.offset
        };
        MMDB_return_s result;
        MMDB_get_value(&country_hash, &result, "code", NULL);
        ok(result.offset > 0, "Found country code for %s", ipstr);
        if (result.offset > 0) {
            ok(MMDB_strcmp_result(mmdb, &result, "US") == 0,
               "Country code is US for %s", ipstr);
        }
        MMDB_get_value(&country_hash, &result, "name", "ascii", NULL);
        if (result.offset > 0) {
            ok(MMDB_strcmp_result(mmdb, &result, "United States") == 0,
               "Country name ascii match United States for %s", ipstr);
        }
        MMDB_get_value(&country_hash, &result, "name", "de", NULL);
        if (result.offset > 0) {
            ok(MMDB_strcmp_result(mmdb, &result, "USA") == 0,
               "Country name de is USA for %s", ipstr);
        }
        MMDB_get_value(&country_hash, &result, "name", "es", NULL);
        if (result.offset > 0) {
            ok(MMDB_strcmp_result(mmdb, &result, "Estados Unidos") == 0,
               "Country name es is Estados Unidos for %s", ipstr);
        }
        MMDB_get_value(&country_hash, &result, "name", "whatever", NULL);
        ok(result.offset == 0,
           "Country name whatever is not found for %s", ipstr);
    }
}

int main(void)
{
    char *fname = get_test_db_fname();
    struct stat sstat;
    int err = stat(fname, &sstat);
    ok(err == 0, "%s exists", fname);

    MMDB_s *mmdb_m = MMDB_open(fname, MMDB_MODE_MEMORY_CACHE);
    ok(mmdb_m != NULL, "MMDB_open successful ( MMDB_MODE_MEMORY_CACHE )");
    if (mmdb_m)
        test_mmdb(mmdb_m);

    MMDB_s *mmdb_s = MMDB_open(fname, MMDB_MODE_STANDARD);
    ok(mmdb_s != NULL, "MMDB_open successful ( MMDB_MODE_STANDARD )");
    if (mmdb_s)
        test_mmdb(mmdb_s);

    done_testing();
}
