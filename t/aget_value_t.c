#include "maxminddb_test_helper.h"

void test_simple_structure(int mode, const char *mode_desc)
{
    const char *filename = "MaxMind-DB-test-decoder.mmdb";
    const char *path = test_database_path(filename);
    MMDB_s *mmdb = open_ok(path, mode, mode_desc);
    free((void *)path);

    const char *ip = "1.1.1.1";
    MMDB_lookup_result_s result =
        lookup_string_ok(mmdb, ip, filename, mode_desc);

    {
        MMDB_entry_data_s entry_data;
        char *lookup_path[] = { "array", "0", NULL };
        int status = MMDB_aget_value(&result.entry, &entry_data, lookup_path);

        cmp_ok(status, "==", MMDB_SUCCESS,
               "status for MMDB_aget_value() is MMDB_SUCCESS - array[0]");
        ok(entry_data.has_data, "found a value with MMDB_aget_value - array[0]");
        cmp_ok(entry_data.type, "==", MMDB_DATA_TYPE_UINT32,
               "returned entry type is uint32 - array[0]");
        cmp_ok(entry_data.uint32, "==", 1, "entry value is 1 - array[0]");
    }

    {
        MMDB_entry_data_s entry_data;
        char *lookup_path[] = { "array", "2", NULL };
        int status = MMDB_aget_value(&result.entry, &entry_data, lookup_path);

        cmp_ok(status, "==", MMDB_SUCCESS,
               "status for MMDB_aget_value() is MMDB_SUCCESS - array[2]");
        ok(entry_data.has_data, "found a value with MMDB_aget_value - array[2]");
        cmp_ok(entry_data.type, "==", MMDB_DATA_TYPE_UINT32,
               "returned entry type is uint32 - array[2]");
        cmp_ok(entry_data.uint32, "==", 3, "entry value is 3 - array[2]");
    }

    MMDB_close(mmdb);
    free(mmdb);
}

void test_nested_structure(int mode, const char *mode_desc)
{
    const char *filename = "MaxMind-DB-test-nested.mmdb";
    const char *path = test_database_path(filename);
    MMDB_s *mmdb = open_ok(path, mode, mode_desc);
    free((void *)path);

    const char *ip = "1.1.1.1";
    MMDB_lookup_result_s result =
        lookup_string_ok(mmdb, ip, filename, mode_desc);

    {
        MMDB_entry_data_s entry_data;
        char *lookup_path[] = { "map1", "map2", "array", "0", "map3", "a", NULL };
        int status = MMDB_aget_value(&result.entry, &entry_data, lookup_path);

        cmp_ok(
            status, "==", MMDB_SUCCESS,
            "status for MMDB_aget_value() is MMDB_SUCCESS - map1{map2}{array}[0]{map3}{a}");
        ok(entry_data.has_data,
           "found a value with MMDB_aget_value - map1{map2}{array}[0]{map3}{a}");
        cmp_ok(entry_data.type, "==", MMDB_DATA_TYPE_UINT32,
               "returned entry type is uint32 - map1{map2}{array}[0]{map3}{a}");
        cmp_ok(entry_data.uint32, "==", 1,
               "entry value is 1 - map1{map2}{array}[0]{map3}{a}");
    }

    {
        MMDB_entry_data_s entry_data;
        char *lookup_path[] = { "map1", "map2", "array", "0", "map3", "c", NULL };
        int status = MMDB_aget_value(&result.entry, &entry_data, lookup_path);

        cmp_ok(
            status, "==", MMDB_SUCCESS,
            "status for MMDB_aget_value() is MMDB_SUCCESS - map1{map2}{array}[0]{map3}{c}");
        ok(entry_data.has_data,
           "found a value with MMDB_aget_value - map1{map2}{array}[0]{map3}{c}");
        cmp_ok(entry_data.type, "==", MMDB_DATA_TYPE_UINT32,
               "returned entry type is uint32 - map1{map2}{array}[0]{map3}{c}");
        cmp_ok(entry_data.uint32, "==", 3,
               "entry value is 3 - map1{map2}{array}[0]{map3}{c}");
    }
}

void run_tests(int mode, const char *mode_desc)
{
    test_simple_structure(mode, mode_desc);
    test_nested_structure(mode, mode_desc);
}

int main(void)
{
    plan(NO_PLAN);
    for_all_modes(&run_tests);
    done_testing();
}
