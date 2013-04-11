#include "MMDB.h"
#include "tap.h"
#include <sys/stat.h>
#include <string.h>
#if HAVE_CONFIG_H
# include <config.h>
#endif

int main(void)
{
    const char *version = MMDB_lib_version();
    ok(version != NULL, "MMDB_lib_version exists");
    if (version)
        ok(strcmp(version, PACKAGE_VERSION) == 0,
           "version is " PACKAGE_VERSION);
    done_testing();
}
