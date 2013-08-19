#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdarg.h>
#include "MMDB.h"
#include "MMDB_test_helper.h"

void for_all_record_sizes(const char *filename_fmt,
                          void (*tests) (int record_size, const char *filename,
                                         const char *description))
{
    int sizes[] = { 24, 28, 32 };
    for (int i = 0; i < 3; i++) {
        int size = sizes[i];

        char filename[500];
        snprintf(filename, 500, filename_fmt, size);

        char description[14];
        snprintf(description, 14, "%i bit record", size);

        tests(size, filename, description);
    }
}

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

    if (NULL == mmdb) {
        BAIL_OUT("could not allocate memory for our MMDB_s struct");
    }

    uint16_t status = MMDB_open(db_file, mode, mmdb);

    int is_ok = ok(MMDB_SUCCESS == status, "open %s status is success - %s",
                   db_file, mode_desc);

    if (!is_ok) {
        diag("open status code = %d", status);
        return NULL;
    }

    is_ok = ok(NULL != mmdb, "returned mmdb struct is not null for %s - %s",
               db_file, mode_desc);

    if (!is_ok) {
        return NULL;
    }

    return mmdb;
}

MMDB_lookup_result_s *lookup_ok(MMDB_s *mmdb, const char *ip,
                                const char *file, const char *mode_desc)
{
    int gai_error, mmdb_error;
    MMDB_lookup_result_s *root = MMDB_lookup(mmdb, ip, &gai_error, &mmdb_error);

    int is_ok = ok(0 == gai_error,
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
    va_start(keys, description);

    MMDB_return_s data;
    int error = MMDB_vget_value(&result->entry, &data, keys);

    va_end(keys);

    ok(!error, "no error from call to MMDB_vget_value - %s", description);
    int is_ok = ok(data.type == expect_type, "got the expected data type - %s",
                   description);
    if (!is_ok) {
        diag("  data type value is %i but expected %i", data.type, expect_type);
    }

    return data;
}
