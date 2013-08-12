#include <netdb.h>
#include "MMDB_test_helper.h"

void run_tests(int mode, const char *mode_desc)
{
    const char *file = "MaxMind-DB-test-ipv4-24.mmdb";
    const char *path = test_database_path(file);
    MMDB_s *mmdb = open_ok(path, mode, mode_desc);

    // All of the remaining tests require an open mmdb
    if (NULL == mmdb) {
        diag("could not open %s - skipping remaining tests", path);
        return;
    }

    ok(37 == mmdb->metadata.node_count, "node_count is 37 - %s", mode_desc);
    ok(24 == mmdb->metadata.record_size, "record_size is 24 - %s", mode_desc);
    ok(4 == mmdb->metadata.ip_version, "ip_version is 4 - %s", mode_desc);
    is(mmdb->metadata.database_type, "Test", "database_type is Test - %s", mode_desc);
    ok(2 == mmdb->metadata.binary_format_major_version, "binary_format_major_version is 2 - %s", mode_desc);
    ok(0 == mmdb->metadata.binary_format_minor_version, "binary_format_minor_version is 0 - %s", mode_desc);
    ok(2 == mmdb->metadata.languages.count, "found 2 languages - %s", mode_desc);
    is(mmdb->metadata.languages.names[0], "en", "first language is en - %s", mode_desc);
    is(mmdb->metadata.languages.names[1], "zh", "second language is zh - %s", mode_desc);
    ok(6 == mmdb->full_record_byte_size, "full_record_byte_size is 6 - %s", mode_desc);

    MMDB_close(mmdb);
}

int main(void)
{
    plan(NO_PLAN);
    for_all_modes(&run_tests);
    done_testing();
}
