#include "maxminddb_test_helper.h"

void run_tests(int mode, const char *mode_desc)
{
    const char *filename = "MaxMind-DB-test-decoder.mmdb";
    const char *path = test_database_path(filename);
    MMDB_s *mmdb = open_ok(path, mode, mode_desc);
    free((void *)path);

    const char *ip = "1.1.1.1";
    MMDB_lookup_result_s result =
        lookup_string_ok(mmdb, ip, filename, mode_desc);

    MMDB_entry_data_s entry_data;
    char *lookup_path[] = {"array", "0", NULL};
    int status = MMDB_aget_value(&result.entry, &entry_data, lookup_path);

    cmp_ok(status, "==", MMDB_SUCCESS,
           "status for MMDB_get_value() is MMDB_SUCCESS");
    ok(entry_data.offset, "found a value with MMDB_aget_value");
    cmp_ok(entry_data.type, "==", MMDB_DATA_TYPE_UINT32,
           "returned entry type is uint32");
    cmp_ok(entry_data.uint32, "==", 1, "entry value is 1");

    MMDB_close(mmdb);
    free(mmdb);
}

int main(void)
{
    plan(NO_PLAN);
    for_all_modes(&run_tests);
    done_testing();
}
