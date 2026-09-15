#include "maxminddb_test_helper.h"

void test_bad_data_size_rejected(void **UNUSED(state)) {
    char *db_file = bad_database_path("libmaxminddb-oversized-array.mmdb");

    MMDB_s mmdb;
    int status = MMDB_open(db_file, MMDB_MODE_MMAP, &mmdb);
    assert_int_equal_desc(status, MMDB_SUCCESS, "opened bad-data-size MMDB");

    int gai_error, mmdb_error;
    MMDB_lookup_result_s result =
        MMDB_lookup_string(&mmdb, "1.2.3.4", &gai_error, &mmdb_error);

    assert_int_equal_desc(mmdb_error, MMDB_SUCCESS, "lookup succeeded");
    assert_true_desc(result.found_entry, "entry found");

    if (result.found_entry) {
        MMDB_entry_data_list_s *entry_data_list = NULL;
        status = MMDB_get_entry_data_list(&result.entry, &entry_data_list);
        assert_int_equal_desc(
            status,
            MMDB_INVALID_DATA_ERROR,
            "MMDB_get_entry_data_list returns INVALID_DATA_ERROR "
            "for array with size exceeding remaining data");
        MMDB_free_entry_data_list(entry_data_list);
    }

    MMDB_close(&mmdb);
    free(db_file);
}

void test_bad_map_size_rejected(void **UNUSED(state)) {
    char *db_file = bad_database_path("libmaxminddb-oversized-map.mmdb");

    MMDB_s mmdb;
    int status = MMDB_open(db_file, MMDB_MODE_MMAP, &mmdb);
    assert_int_equal_desc(status, MMDB_SUCCESS, "opened bad-map-size MMDB");

    int gai_error, mmdb_error;
    MMDB_lookup_result_s result =
        MMDB_lookup_string(&mmdb, "1.2.3.4", &gai_error, &mmdb_error);

    assert_int_equal_desc(mmdb_error, MMDB_SUCCESS, "lookup succeeded");
    assert_true_desc(result.found_entry, "entry found");

    if (result.found_entry) {
        MMDB_entry_data_list_s *entry_data_list = NULL;
        status = MMDB_get_entry_data_list(&result.entry, &entry_data_list);
        assert_int_equal_desc(
            status,
            MMDB_INVALID_DATA_ERROR,
            "MMDB_get_entry_data_list returns INVALID_DATA_ERROR "
            "for map with size exceeding remaining data");
        MMDB_free_entry_data_list(entry_data_list);
    }

    MMDB_close(&mmdb);
    free(db_file);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_bad_data_size_rejected),
        cmocka_unit_test(test_bad_map_size_rejected),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
