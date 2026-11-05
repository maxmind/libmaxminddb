#include "maxminddb_test_helper.h"

// The repeated-key databases from MaxMind-DB. This test needs no temporary
// file, so it also runs on Windows.
static void test_bad_data_file(const char *filename) {
    char *path = bad_database_path(filename);
    MMDB_s mmdb;
    int status = MMDB_open(path, MMDB_MODE_MMAP, &mmdb);
    cmp_ok(status,
           "==",
           MMDB_INVALID_METADATA_ERROR,
           "MMDB_open rejects %s",
           filename);
    if (MMDB_SUCCESS == status) {
        MMDB_close(&mmdb);
    }
    free(path);
}

int main(void) {
    plan(NO_PLAN);
    test_bad_data_file("libmaxminddb-repeated-metadata-key.mmdb");
    test_bad_data_file("libmaxminddb-repeated-description-key.mmdb");
    done_testing();
}
