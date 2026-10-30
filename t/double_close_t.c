#include "maxminddb_test_helper.h"

void test_double_close(void) {
    char *db_file = test_database_path("MaxMind-DB-test-ipv4-24.mmdb");

    MMDB_s mmdb;
    int status = MMDB_open(db_file, MMDB_MODE_MMAP, &mmdb);
    cmp_ok(status, "==", MMDB_SUCCESS, "MMDB_open succeeded");

    if (status != MMDB_SUCCESS) {
        free(db_file);
        return;
    }
    uint32_t saved_flags = mmdb.flags;

    /* First close should work normally */
    MMDB_close(&mmdb);

    ok(mmdb.file_content == NULL, "file_content is NULL after first close");
    ok(mmdb.data_section == NULL, "data_section is NULL after close");
    ok(mmdb.metadata_section == NULL, "metadata_section is NULL after close");
    cmp_ok(mmdb.metadata.languages.count,
           "==",
           0,
           "languages.count is 0 after close");
    cmp_ok(mmdb.metadata.description.count,
           "==",
           0,
           "description.count is 0 after close");
    cmp_ok(mmdb.file_size, "==", 0, "file_size is 0 after close");
    cmp_ok(
        mmdb.data_section_size, "==", 0, "data_section_size is 0 after close");
    cmp_ok(mmdb.metadata_section_size,
           "==",
           0,
           "metadata_section_size is 0 after close");

    /* Second close should be a safe no-op (file_content was NULLed) */
    MMDB_close(&mmdb);

    ok(1, "calling MMDB_close twice does not crash");

    status = MMDB_open(db_file, saved_flags, &mmdb);
    cmp_ok(
        status, "==", MMDB_SUCCESS, "reopen accepts flags from an open reader");
    MMDB_close(&mmdb);
    free(db_file);
}

static void test_empty_and_failed_open(void) {
    MMDB_s mmdb = {0};
    MMDB_close(&mmdb);
    ok(mmdb.file_content == NULL, "closing a zeroed reader is safe");

    char *paths[] = {
        test_database_path("does-not-exist.mmdb"),
        bad_database_path("libmaxminddb-metadata-marker-only.mmdb")};
    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) {
        for (unsigned int fill = 0; fill <= 0xa5; fill += 0xa5) {
            memset(&mmdb, (int)fill, sizeof(mmdb));
            int status = MMDB_open(paths[i], MMDB_MODE_MMAP, &mmdb);
            ok(status != MMDB_SUCCESS,
               "invalid database fails to open - fill %x - %s",
               fill,
               paths[i]);
            ok(mmdb.file_content == NULL && mmdb.filename == NULL,
               "failed open releases the reader - fill %x - %s",
               fill,
               paths[i]);
            MMDB_close(&mmdb);
            MMDB_close(&mmdb);
            ok(mmdb.file_content == NULL && mmdb.filename == NULL,
               "failed reader tolerates repeated close - fill %x - %s",
               fill,
               paths[i]);
        }
        free(paths[i]);
    }
}

int main(void) {
    plan(NO_PLAN);
    test_double_close();
    test_empty_and_failed_open();
    done_testing();
}
