#include "maxminddb_test_helper.h"

void run_tests(int mode, const char *mode_desc) {
    const char *filename = "MaxMind-DB-string-value-entries.mmdb";
    char *path = test_database_path(filename);
    MMDB_s *mmdb = open_ok(path, mode, mode_desc);
    free(path);

    const char *ip = "1.1.1.1";
    MMDB_lookup_result_s result =
        lookup_string_ok(mmdb, ip, filename, mode_desc);

    MMDB_entry_data_s entry_data;
    int status = MMDB_get_value(&result.entry, &entry_data, NULL);

    assert_int_equal_desc(
        status, MMDB_SUCCESS, "status for MMDB_get_value() is MMDB_SUCCESS");
    assert_true_desc(entry_data.has_data,
                     "found a value when varargs list is just NULL");
    assert_int_equal_desc(entry_data.type,
                          MMDB_DATA_TYPE_UTF8_STRING,
                          "returned entry type is utf8_string");

    MMDB_close(mmdb);
    free(mmdb);
}

static void test_no_map_get_value(void **UNUSED(state)) {
    for_all_modes(&run_tests);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_no_map_get_value),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
