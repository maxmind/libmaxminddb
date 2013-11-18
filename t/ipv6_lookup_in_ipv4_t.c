#include "maxminddb_test_helper.h"

void run_tests(int mode, const char *mode_desc)
{
    const char *filename = "MaxMind-DB-test-ipv4-28.mmdb";
    const char *path = test_database_path(filename);
    MMDB_s *mmdb = open_ok(path, mode, mode_desc);
    free((void *)path);

    const char *ip = "::abcd";
    int gai_error, mmdb_error;
    MMDB_lookup_result_s UNUSED(result) =
        MMDB_lookup_string(mmdb, ip, &gai_error, &mmdb_error);

    cmp_ok(
        mmdb_error, "==", MMDB_IPV6_LOOKUP_IN_IPV4_DATABASE_ERROR,
        "MMDB_lookup_string sets mmdb_error to MMDB_IPV6_LOOKUP_IN_IPV4_DATABASE_ERROR when we try to look up an IPv6 address in an IPv4-only database");

    MMDB_close(mmdb);
    free(mmdb);
}

int main(void)
{
    plan(NO_PLAN);
    for_all_modes(&run_tests);
    done_testing();
}
