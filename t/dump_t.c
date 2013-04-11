#include "MMDB.h"
#include "tap.h"
#include <sys/stat.h>
#include <arpa/inet.h>
#include <string.h>

int main(void)
{
    char *fname = "./data/test-database.dat";
    struct stat sstat;
    int err = stat(fname, &sstat);
    ok(err == 0, "%s exists", fname);

    MMDB_s *mmdb = MMDB_open(fname, MMDB_MODE_MEMORY_CACHE);
    ok(mmdb != NULL, "MMDB_open successful");
    if (mmdb) {

        MMDB_root_entry_s root = {.entry.mmdb = mmdb };
        char *ipstr = "24.24.24.24";
        uint32_t ipnum = ip_to_num(ipstr);
        err = MMDB_lookup_by_ipnum(ipnum, &root);
        ok(err == MMDB_SUCCESS, "Search for %s SUCCESSFUL", ipstr);
        ok(root.entry.offset > 0, "Found something %s good", ipstr);
        MMDB_decode_all_s *decode_all = calloc(1, sizeof(MMDB_decode_all_s));
        int err = MMDB_get_tree(&root.entry, &decode_all);

        if (decode_all != NULL)
            MMDB_dump(decode_all, 0);
    }
    done_testing();
}
