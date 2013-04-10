#include "MMDB.h"
#include "tap.h"
#include <sys/stat.h>
#include <arpa/inet.h>
#include <string.h>

int main(void)
{
    char *fname = getenv("MMDB_TEST_DATABASE");
    if (!fname)
        fname = MMDB_DEFAULT_DATABASE;

    struct stat sstat;
    int err = stat(fname, &sstat);
    ok(err == 0, "%s exists", fname);

    MMDB_s *mmdb = MMDB_open(fname, MMDB_MODE_MEMORY_CACHE);
    // MMDB_s *mmdb = MMDB_open(fname, MMDB_MODE_STANDARD);
    ok(mmdb != NULL, "MMDB_open successful");
    if (mmdb) {

        MMDB_decode_all_s *decode_all = calloc(1, sizeof(MMDB_decode_all_s));
        int err = MMDB_get_tree(&mmdb->meta, &decode_all);

        if (decode_all != NULL)
            MMDB_dump(decode_all, 0);
    }
    done_testing();
}
