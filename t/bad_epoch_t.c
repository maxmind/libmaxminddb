#include "maxminddb_test_helper.h"

void test_bad_epoch(void **UNUSED(state)) {
    char *db_file = bad_database_path("libmaxminddb-uint64-max-epoch.mmdb");

    /* Verify we can at least open the DB without crashing */
    MMDB_s mmdb;
    int status = MMDB_open(db_file, MMDB_MODE_MMAP, &mmdb);
    assert_int_equal_desc(status, MMDB_SUCCESS, "opened bad-epoch MMDB");

    /* Run mmdblookup --verbose via system() and check it doesn't crash.
     * We redirect output to /dev/null; the return code tells us
     * whether the process survived. Try both possible paths since tests
     * may run from either the project root or the t/ directory. */
    char cmd[512];
    const char *binary = "../bin/mmdblookup";
    FILE *test_bin = fopen(binary, "r");
    if (!test_bin) {
        binary = "./bin/mmdblookup";
        test_bin = fopen(binary, "r");
    }
    if (test_bin) {
        fclose(test_bin);
    }

    if (!test_bin) {
        print_message("mmdblookup binary not found\n");
        MMDB_close(&mmdb);
        free(db_file);
        skip();
    }
    snprintf(cmd,
             sizeof(cmd),
             "%s -f %s -i 1.2.3.4 -v > /dev/null 2>&1",
             binary,
             db_file);
    int ret = system(cmd);
    /* system() returns the exit status; a signal-killed process gives
     * a non-zero status. WIFEXITED checks for normal exit. */
    assert_true_desc(
        WIFEXITED(ret) && WEXITSTATUS(ret) == 0,
        "mmdblookup --verbose with UINT64_MAX build_epoch does not crash");

    MMDB_close(&mmdb);
    free(db_file);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_bad_epoch),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
