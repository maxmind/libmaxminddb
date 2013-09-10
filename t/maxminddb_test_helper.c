#include <assert.h>
#include <libgen.h>
#include <netdb.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "maxminddb.h"
#include "maxminddb_test_helper.h"

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
    tests(MMDB_MODE_MMAP, "mmap mode");
    tests(MMDB_MODE_MEMORY_CACHE, "memory cache mode");
}

const char *test_database_path(const char *filename)
{
    char cwd[500];
    getcwd(cwd, 500);

    char *test_db_dir;
    if (strcmp(basename(cwd), "t") == 0) {
        test_db_dir = "../maxmind-db/test-data";
    } else {
        test_db_dir = "./maxmind-db/test-data";
    }

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
        free(mmdb);
        return NULL;
    }

    is_ok = ok(NULL != mmdb, "returned mmdb struct is not null for %s - %s",
               db_file, mode_desc);

    if (!is_ok) {
        free(mmdb);
        return NULL;
    }

    return mmdb;
}

MMDB_lookup_result_s lookup_string_ok(MMDB_s *mmdb, const char *ip,
                                      const char *file, const char *mode_desc)
{
    int gai_error, mmdb_error;
    MMDB_lookup_result_s result =
        MMDB_lookup_string(mmdb, ip, &gai_error, &mmdb_error);

    test_lookup_errors(gai_error, mmdb_error, "MMDB_lookup_string", ip, file,
                       mode_desc);

    return result;
}

MMDB_lookup_result_s lookup_sockaddr_ok(MMDB_s *mmdb, const char *ip,
                                        const char *file, const char *mode_desc)
{
    int ai_flags = AI_NUMERICHOST;
    struct addrinfo hints = {
        .ai_socktype = SOCK_STREAM
    };
    struct addrinfo *addresses;

    if (ip[0] == ':') {
        hints.ai_flags = ai_flags | AI_V4MAPPED;
        hints.ai_family = AF_INET6;
    } else {
        hints.ai_flags = ai_flags;
        hints.ai_family = AF_INET;
    }

    int gai_error = getaddrinfo(ip, NULL, &hints, &addresses);

    int mmdb_error = 0;
    MMDB_lookup_result_s result;
    if (gai_error == 0) {
        result = MMDB_lookup_sockaddr(mmdb, addresses->ai_addr, &mmdb_error);
    }
    freeaddrinfo(addresses);

    test_lookup_errors(gai_error, mmdb_error, "MMDB_lookup_sockaddr", ip, file,
                       mode_desc);

    return result;
}

void test_lookup_errors(int gai_error, int mmdb_error,
                        const char *function, const char *ip,
                        const char *file, const char *mode_desc)
{

    int is_ok = ok(0 == gai_error,
                   "no getaddrinfo error in call to %s for %s - %s - %s",
                   function, ip, file, mode_desc);

    if (!is_ok) {
        diag("error from call to getaddrinfo for %s - %s",
             ip, gai_strerror(gai_error));
    }

    is_ok = ok(0 == mmdb_error,
               "no MMDB error in call to %s for %s - %s - %s",
               function, ip, file, mode_desc);

    if (!is_ok) {
        diag("MMDB error - %d", mmdb_error);
    }
}

MMDB_entry_data_s data_ok(MMDB_lookup_result_s *result, int expect_type,
                          const char *description, ...)
{
    va_list keys;
    va_start(keys, description);

    MMDB_entry_data_s data;
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

void compare_double(double got, double expect)
{
    double diff = fabs(got - expect);
    int is_ok = ok(diff < 0.01, "double value was approximately %2.6f", expect);
    if (!is_ok) {
        diag("  got %2.6f but expected %2.6f (diff = %2.6f)",
             got, expect, diff);
    }
}

void compare_float(float got, float expect)
{
    float diff = fabsf(got - expect);
    int is_ok = ok(diff < 0.01, "float value was approximately %2.1f", expect);
    if (!is_ok) {
        diag("  got %2.4f but expected %2.1f (diff = %2.1f)",
             got, expect, diff);
    }
}
