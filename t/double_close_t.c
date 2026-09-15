#include "maxminddb_test_helper.h"

void test_double_close(void **UNUSED(state)) {
    char *db_file = test_database_path("MaxMind-DB-test-ipv4-24.mmdb");

    MMDB_s mmdb;
    int status = MMDB_open(db_file, MMDB_MODE_MMAP, &mmdb);
    free(db_file);
    assert_int_equal_desc(status, MMDB_SUCCESS, "MMDB_open succeeded");

    /* First close should work normally */
    MMDB_close(&mmdb);

    assert_true_desc(mmdb.file_content == NULL,
                     "file_content is NULL after first close");
    assert_true_desc(mmdb.data_section == NULL,
                     "data_section is NULL after close");
    assert_true_desc(mmdb.metadata_section == NULL,
                     "metadata_section is NULL after close");
    assert_int_equal_desc(
        mmdb.metadata.languages.count, 0, "languages.count is 0 after close");
    assert_int_equal_desc(mmdb.metadata.description.count,
                          0,
                          "description.count is 0 after close");
    assert_int_equal_desc(mmdb.file_size, 0, "file_size is 0 after close");
    assert_int_equal_desc(
        mmdb.data_section_size, 0, "data_section_size is 0 after close");
    assert_int_equal_desc(mmdb.metadata_section_size,
                          0,
                          "metadata_section_size is 0 after close");

    /* Second close should be a safe no-op (file_content was NULLed) */
    MMDB_close(&mmdb);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_double_close),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
