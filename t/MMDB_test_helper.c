#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdarg.h>
#include "MMDB.h"
#include "MMDB_test_helper.h"

void for_all_modes( void (*tests)(int mode, const char *description) )
{
    tests(MMDB_MODE_STANDARD, "standard mode");
    tests(MMDB_MODE_MEMORY_CACHE, "memory cache mode");
}

#define MAX_DESCRIPTION_LENGTH 500

MMDB_s *open_ok(char *db_file, int mode, char *mode_desc)
{
    MMDB_s *mmdb;
    uint16_t status;
    char desc[MAX_DESCRIPTION_LENGTH];
    
    status =
        MMDB_open(db_file, mode, &mmdb);

    {
        snprintf_or_bail(desc, MAX_DESCRIPTION_LENGTH,
                         "open %s status is success - %s", db_file,
                         mode_desc);
        if (!ok(MMDB_SUCCESS == status, desc)) {
            snprintf_or_bail(desc, MAX_DESCRIPTION_LENGTH,
                             "open status code = %d", status);
            diag(desc);
        }
    }

    {
        snprintf_or_bail(desc, MAX_DESCRIPTION_LENGTH,
                         "returned mmdb struct is not null for %s - %s",db_file,
                         mode_desc);
        if (!ok(NULL != mmdb, desc)) {
            // All of the remaining tests require an open mmdb
            return NULL;
        }
    }

    return mmdb;
}

void snprintf_or_bail(char *target, size_t size, char *fmt, ...)
{
    va_list args;
    int ok;

    va_start(args, fmt);
    ok = vsnprintf(target, size, fmt, args);
    va_end(args);

    if (!ok) {
        BAIL_OUT("sprintf failed");
    }
}
