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
    is(mmdb->metadata.database_type, "Test", "database_type is Test - %s",
       mode_desc);
    // 2013-07-01T00:00:00Z
    uint64_t expect_epoch = 1372636800;
    int is_ok =
        ok(mmdb->metadata.build_epoch >= expect_epoch, "build_epoch > %lli",
           expect_epoch);
    if (!is_ok) {
        diag("  epoch is %lli", mmdb->metadata.build_epoch);
    }

    ok(2 == mmdb->metadata.binary_format_major_version,
       "binary_format_major_version is 2 - %s", mode_desc);
    ok(0 == mmdb->metadata.binary_format_minor_version,
       "binary_format_minor_version is 0 - %s", mode_desc);

    ok(2 == mmdb->metadata.languages.count, "found 2 languages - %s",
       mode_desc);
    is(mmdb->metadata.languages.names[0], "en", "first language is en - %s",
       mode_desc);
    is(mmdb->metadata.languages.names[1], "zh", "second language is zh - %s",
       mode_desc);

    ok(2 == mmdb->metadata.description.count, "found 2 descriptions - %s",
       mode_desc);
    for (int i = 0; i < mmdb->metadata.description.count; i++) {
        const char *language =
            mmdb->metadata.description.descriptions[i]->language;
        const char *description =
            mmdb->metadata.description.descriptions[i]->description;
        if (strncmp(language, "en", 2) == 0) {
            ok(1, "found en description");
            is(description, "Test Database", "en description - %s");
        } else if (strncmp(language, "zh", 2) == 0) {
            ok(1, "found zh description");
            is(description, "Test Database Chinese", "zh description - %s");
        } else {
            ok(0, "found unknown description in unexpected language - %s",
               language);
        }
    }

    ok(6 == mmdb->full_record_byte_size, "full_record_byte_size is 6 - %s",
       mode_desc);

    free((void *)path);
    MMDB_close(mmdb);
}

int main(void)
{
    plan(NO_PLAN);
    for_all_modes(&run_tests);
    done_testing();
}
