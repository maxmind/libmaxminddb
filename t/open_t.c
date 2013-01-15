#include "MMDB.h"
#include "tap.h"
#include <sys/stat.h>

int main(void)
{
    char *fname = "./data/test-database.dat";
    struct stat sstat;
    int err = stat(fname, &sstat);
    ok(err == 0, "%s exists", fname);

    MMDB_s *mmdb = MMDB_open(fname, MMDB_MODE_MEMORY_CACHE);
    ok(mmdb != NULL, "MMDB_open successful");
    if (mmdb) {

        ok((mmdb->recbits == 24 || mmdb->recbits == 28
            || mmdb->recbits == 32), "recbits = %d", mmdb->recbits);
    }
    done_testing();
}
