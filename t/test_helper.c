#include <stdlib.h>
#include "MMDB.h"
#include "test_helper.h"

char *get_test_db_fname(void)
{
    char *fname = getenv("MMDB_TEST_DATABASE");
    if (!fname)
        fname = MMDB_DEFAULT_DATABASE;
    return fname;
}
