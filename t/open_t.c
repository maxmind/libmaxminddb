#include "MMDB.h"
#include "tap.h"
#include <sys/stat.h>

int main(void)
{
    char *fname = "./data/test-database.dat";
    struct stat sstat;
    int err = stat(fname, &sstat);
    ok(err == 0, "%s exists", fname);

    MMDB_s *mmdb_m = MMDB_open(fname, MMDB_MODE_MEMORY_CACHE);
    ok(mmdb_m != NULL, "MMDB_open successful ( MMDB_MODE_MEMORY_CACHE )");

    MMDB_s *mmdb_s = MMDB_open(fname, MMDB_MODE_STANDARD);
    ok(mmdb_s != NULL, "MMDB_open successful ( MMDB_MODE_STANDARD )");

    if (mmdb_m && mmdb_s) {
        int rbm = mmdb_m->recbits;
        int rbs = mmdb_s->recbits;

        ok((rbs == 24 || rbs == 28
            || rbs == 32), "recbits = %d MMDB_MODE_STANDARD", rbs);
        ok((rbm == 24 || rbm == 28
            || rbm == 32), "recbits = %d MMDB_MODE_MEMORY_CACHE", rbm);

        ok(rbm == rbs, "recbits are the same");

        ok(mmdb_s->node_count == mmdb_m->node_count,
           "node_count are the same ( %d == %d )", mmdb_m->node_count,
           mmdb_s->node_count);

    }
    done_testing();
}
