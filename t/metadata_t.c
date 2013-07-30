#include <netdb.h>
#include "MMDB_test_helper.h"

void run_tests()
{
    const char *file = "MaxMind-DB-test-ipv4-24.mmdb";
    const char *path = test_database_path(file);
    MMDB_s *mmdb = open_ok(path, mode, mode_desc);

    // All of the remaining tests require an open mmdb
    if (NULL == mmdb) {
        diag("could not open %s - skipping remaining tests", path);
        return;
    }

    {
        const char *ip = "not an ip";
        int gai_error, mmdb_error;
        MMDB_root_entry_s *root;

        root = MMDB_lookup(mmdb, ip, &gai_error, &mmdb_error);

        ok(EAI_NONAME == gai_error,
           "MMDB_lookup populates getaddrinfo error properly - %s", ip);

        ok(NULL == root,
           "no root entry struct returned for invalid IP address '%s'", ip);
    }

    {
        const char *ip = "e900::";
        int gai_error, mmdb_error;
        MMDB_root_entry_s *root;

        root = lookup_ok(mmdb, ip, file, mode_desc);

        ok(NULL == root,
           "no root entry struct returned for IP address not in the database - %s - %s - %s",
           ip, file, mode_desc);
    }

    {
        const char *ip = "::1.1.1.1";
        int gai_error, mmdb_error;
        MMDB_root_entry_s *root;
        MMDB_return_s *result;

        root = lookup_ok(mmdb, ip, file, mode_desc);

        ok(NULL != root,
           "got a root entry struct for IP address in the database - %s - %s - %s",
           ip, file, mode_desc);

        ok(root->entry.offset > 0,
           "root.entry.offset > 0 for address in the database - %s - %s - %s",
           ip, file, mode_desc);

        result = MMDB_get_value(
    }

    MMDB_close(mmdb);
}

int main(void)
{
    plan(NO_PLAN);
    run_tests();
    done_testing();
}
