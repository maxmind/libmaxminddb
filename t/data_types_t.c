#include "MMDB_test_helper.h"

void run_tests(int mode, const char *mode_desc)
{
    char *db_file = "../maxmind-db/test-data/MaxMind-DB-test-decoder.mmdb";
    MMDB_s *mmdb = open_ok(db_file, mode, mode_desc);

    if (NULL == mmdb) {
        diag("Could not open %s - skipping remaining tests", db_file);
        return;
    }
}

int main(void)
{
    plan(NO_PLAN);
    for_all_modes(&run_tests);
    done_testing();
}
