#include "maxminddb_test_helper.h"

/* This test exercises a bug found in MMDB_get_value for certain types of
 * nested data structures which contain pointers. See
 * https://github.com/maxmind/libmaxminddb/issues/2 and
 * https://github.com/maxmind/libmaxminddb/issues/3 */

void run_tests(int mode, const char *mode_desc)
{
    const char *filename = "GeoIP2-City-Test.mmdb";
    const char *path = test_database_path(filename);
    MMDB_s *mmdb = open_ok(path, mode, mode_desc);
    free((void *)path);

    const char *ip = "2001:218::";
    MMDB_lookup_result_s result =
        lookup_string_ok(mmdb, ip, filename, mode_desc);

    MMDB_entry_data_s entry_data;
    char *lookup_path[] = { "country", "iso_code", NULL };
    int status = MMDB_aget_value(&result.entry, &entry_data, lookup_path);
    cmp_ok( status, "==", MMDB_SUCCESS, "lookup for country{iso_code} was successful");
    ok(entry_data.has_data, "found data for country{iso_code}");
    ok(strcmp(entry_data.utf8_string, "JP") == 0, "iso_code is JP");
}

int main(void)
{
    plan(NO_PLAN);
    for_all_modes(&run_tests);
    done_testing();
}
