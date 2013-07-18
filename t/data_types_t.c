#include "MMDB_test_helper.h"

void run_tests(int mode, const char *description)
{
    MMDB_s *mmdb =
        MMDB_open("../maxmind-db/test-data/MaxMind-DB-test-decoder.mmdb", mode);
    char *d = malloc(500);

    {
        if (!snprintf
            (d, 500, "opened MaxMind-DB-test-decoder.mmdb - %s", description)) {
            BAIL_OUT("sprintf failed");
        }
        ok(mmdb != NULL, d);
    }

    {
        char *ip = "::1.1.1.1";
    }        

    free(d);
}

int main(void)
{
    plan(NO_PLAN);
    for_all_modes(&run_tests);
    done_testing();
}
