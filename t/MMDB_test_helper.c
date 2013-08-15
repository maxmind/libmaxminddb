#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdarg.h>
#include "MMDB.h"
#include "MMDB_test_helper.h"

void for_all_modes(void (*tests) (int mode, const char *description))
{
    tests(MMDB_MODE_STANDARD, "standard mode");
    tests(MMDB_MODE_MEMORY_CACHE, "memory cache mode");
}

static const char *test_db_dir = "../maxmind-db/test-data";

const char *test_database_path(const char *filename)
{
    char *path = malloc(500);
    assert(path != NULL);

    snprintf(path, 500, "%s/%s", test_db_dir, filename);

    return (const char *)path;
}

MMDB_s *open_ok(const char *db_file, int mode, const char *mode_desc)
{
    MMDB_s *mmdb = (MMDB_s *)calloc(1, sizeof(MMDB_s));
    uint16_t status;
    int ok;

    if (NULL == mmdb) {
        BAIL_OUT("could not allocate memory for our MMDB_s struct");
    }

    status = MMDB_open(db_file, mode, mmdb);

    ok = ok(MMDB_SUCCESS == status, "open %s status is success - %s",
            db_file, mode_desc);

    if (!ok) {
        diag("open status code = %d", status);
        return NULL;
    }

    ok = ok(NULL != mmdb, "returned mmdb struct is not null for %s - %s",
            db_file, mode_desc);

    if (!ok) {
        return NULL;
    }

    return mmdb;
}

MMDB_lookup_result_s *lookup_ok(MMDB_s *mmdb, const char *ip,
                                const char *file, const char *mode_desc)
{
    int gai_error, mmdb_error;
    MMDB_lookup_result_s *root;
    int is_ok;

    root = MMDB_lookup(mmdb, ip, &gai_error, &mmdb_error);

    is_ok = ok(0 == gai_error,
               "no getaddrinfo error in call to MMDB_lookup for %s - %s - %s",
               ip, file, mode_desc);

    if (!is_ok) {
        diag("error from call to getaddrinfo for %s - %s",
             ip, gai_strerror(gai_error));
    }

    is_ok = ok(0 == mmdb_error,
               "no MMDB error in call to MMDB_lookup for %s - %s - %s",
               ip, file, mode_desc);

    if (!is_ok) {
        diag("MMDB error - %d", mmdb_error);
    }

    return root;
}

MMDB_return_s data_ok(MMDB_lookup_result_s *result, int expect_type,
                      const char *description, ...)
{
    va_list keys;
    MMDB_return_s data;
    va_start(keys, expect_type);
    int error = MMDB_vget_value(&result->entry, &data, keys);
    va_end(keys);

    ok(!error, "no error from call to MMDB_vget_value - %s", description);
    int is_ok =
        ok(data.type == expect_type, "got the expected data type - %s",
           description);
    if (!is_ok) {
        diag("  data type value is %i but expected %i", data.type, expect_type);
    }

    return data;
}
