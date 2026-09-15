#include "maxminddb_test_helper.h"

#if HAVE_CONFIG_H
    #include <config.h>
#endif

#include <assert.h>
#include <stdarg.h>
#include <sys/types.h>

#include "maxminddb.h"

#ifdef _WIN32
    #include <io.h>
#else
    #include <libgen.h>
    #include <unistd.h>
#endif

void for_all_record_sizes(const char *filename_fmt,
                          void (*tests)(int record_size,
                                        const char *filename,
                                        const char *description)) {
    int sizes[] = {24, 28, 32};
    for (int i = 0; i < 3; i++) {
        int size = sizes[i];

        char filename[500];
#if defined(__clang__)
    // This warning seems ok to ignore here in the tests.
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif
        snprintf(filename, 500, filename_fmt, size);
#if defined(__clang__)
    #pragma clang diagnostic pop
#endif

        char description[14];
        snprintf(description, 14, "%i bit record", size);

        tests(size, filename, description);
    }
}

void for_all_modes(void (*tests)(int mode, const char *description)) {
    tests(MMDB_MODE_MMAP, "mmap mode");
}

char *test_database_path(const char *filename) {
    char *test_db_dir;
#ifdef _WIN32
    test_db_dir = "../t/maxmind-db/test-data";
#else
    char cwd[500];
    char *UNUSED(tmp) = getcwd(cwd, 500);

    if (strcmp(basename(cwd), "t") == 0) {
        test_db_dir = "./maxmind-db/test-data";
    } else {
        test_db_dir = "./t/maxmind-db/test-data";
    }
#endif

    char *path = malloc(500);
    assert(NULL != path);

    snprintf(path, 500, "%s/%s", test_db_dir, filename);

    return path;
}

char *bad_database_path(const char *filename) {
    char *bad_db_dir;
#ifdef _WIN32
    bad_db_dir = "../t/maxmind-db/bad-data/libmaxminddb";
#else
    char cwd[500];
    char *UNUSED(tmp) = getcwd(cwd, 500);

    if (strcmp(basename(cwd), "t") == 0) {
        bad_db_dir = "./maxmind-db/bad-data/libmaxminddb";
    } else {
        bad_db_dir = "./t/maxmind-db/bad-data/libmaxminddb";
    }
#endif

    char *path = malloc(500);
    assert(NULL != path);

    snprintf(path, 500, "%s/%s", bad_db_dir, filename);

    return path;
}

char *dup_entry_string_or_bail(MMDB_entry_data_s entry_data) {
    char *string = mmdb_strndup(entry_data.utf8_string, entry_data.data_size);
    if (NULL == string) {
        fail_msg("mmdb_strndup failed");
    }

    return string;
}

MMDB_s *open_ok(const char *db_file, int mode, const char *mode_desc) {
#ifdef _WIN32
    int access_rv = _access(db_file, 04);
#else
    int access_rv = access(db_file, R_OK);
#endif
    if (access_rv != 0) {
        fail_msg("could not read the specified file - %s\nIf you are in a git "
                 "checkout try running 'git submodule update --init'",
                 db_file);
    }

    MMDB_s *mmdb = (MMDB_s *)calloc(1, sizeof(MMDB_s));

    if (NULL == mmdb) {
        fail_msg("could not allocate memory for our MMDB_s struct");
    }

    int status = MMDB_open(db_file, (uint32_t)mode, mmdb);

    assert_int_equal_desc(status,
                          MMDB_SUCCESS,
                          "open %s status is success - %s (%s)",
                          db_file,
                          mode_desc,
                          MMDB_strerror(status));
    assert_true_desc(mmdb->file_size > 0,
                     "mmdb struct has been set for %s - %s",
                     db_file,
                     mode_desc);

    return mmdb;
}

MMDB_lookup_result_s lookup_string_ok(MMDB_s *mmdb,
                                      const char *ip,
                                      const char *file,
                                      const char *mode_desc) {
    int gai_error, mmdb_error;
    MMDB_lookup_result_s result =
        MMDB_lookup_string(mmdb, ip, &gai_error, &mmdb_error);

    test_lookup_errors(
        gai_error, mmdb_error, "MMDB_lookup_string", ip, file, mode_desc);

    return result;
}

MMDB_lookup_result_s lookup_sockaddr_ok(MMDB_s *mmdb,
                                        const char *ip,
                                        const char *file,
                                        const char *mode_desc) {
    int ai_flags = AI_NUMERICHOST;
    struct addrinfo hints = {.ai_socktype = SOCK_STREAM};
    struct addrinfo *addresses = NULL;

    if (ip[0] == ':') {
        hints.ai_flags = ai_flags;
#if defined AI_V4MAPPED && !defined __FreeBSD__
        hints.ai_flags |= AI_V4MAPPED;
#endif
        hints.ai_family = AF_INET6;
    } else {
        hints.ai_flags = ai_flags;
        hints.ai_family = AF_INET;
    }

    int gai_error = getaddrinfo(ip, NULL, &hints, &addresses);

    int mmdb_error = 0;
    MMDB_lookup_result_s result = {.found_entry = false};
    if (gai_error == 0) {
        result = MMDB_lookup_sockaddr(mmdb, addresses->ai_addr, &mmdb_error);
    }
    if (NULL != addresses) {
        freeaddrinfo(addresses);
    }

    test_lookup_errors(
        gai_error, mmdb_error, "MMDB_lookup_sockaddr", ip, file, mode_desc);

    return result;
}

void test_lookup_errors(int gai_error,
                        int mmdb_error,
                        const char *function,
                        const char *ip,
                        const char *file,
                        const char *mode_desc) {
    assert_int_equal_desc(gai_error,
                          0,
                          "no getaddrinfo error in call to %s for %s - %s - %s "
                          "(%s)",
                          function,
                          ip,
                          file,
                          mode_desc,
                          gai_strerror(gai_error));
    assert_int_equal_desc(mmdb_error,
                          MMDB_SUCCESS,
                          "no MMDB error in call to %s for %s - %s - %s (%s)",
                          function,
                          ip,
                          file,
                          mode_desc,
                          MMDB_strerror(mmdb_error));
}

MMDB_entry_data_s data_ok(MMDB_lookup_result_s *result,
                          uint32_t expect_type,
                          const char *description,
                          ...) {
    va_list keys;
    va_start(keys, description);

    MMDB_entry_data_s data;
    int status = MMDB_vget_value(&result->entry, &data, keys);

    va_end(keys);

    assert_int_equal_desc(status,
                          MMDB_SUCCESS,
                          "no error from call to MMDB_vget_value - %s (%s)",
                          description,
                          MMDB_strerror(status));
    assert_int_equal_desc(
        data.type, expect_type, "got the expected data type - %s", description);

    return data;
}

void compare_double(double got, double expect) {
    assert_true_desc(fabs(got - expect) < 0.01,
                     "double value %2.6f is approximately %2.6f",
                     got,
                     expect);
}

void compare_float(float got, float expect) {
    assert_true_desc(fabsf(got - expect) < 0.01,
                     "float value %2.4f is approximately %2.1f",
                     got,
                     expect);
}
