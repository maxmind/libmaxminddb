#define _XOPEN_SOURCE 700

#include "maxminddb_test_helper.h"

#include <stdlib.h>

// One IPv4 node whose 24-bit records both mark "no data", the data section
// separator, and the metadata marker.
#define DB_PREFIX                                                              \
    "\x00\x00\x01\x00\x00\x01"                                                 \
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"         \
    "\xAB\xCD\xEF"                                                             \
    "MaxMind.com"

// The metadata keys that MMDB_open requires, except description, without the
// map header. database_type starts at metadata offset 0x29.
#define REQUIRED_KEYS_BEFORE_DESCRIPTION                                       \
    "\x4A"                                                                     \
    "node_count"                                                               \
    "\xC1\x01"                                                                 \
    "\x4B"                                                                     \
    "record_size"                                                              \
    "\xA1\x18"                                                                 \
    "\x4A"                                                                     \
    "ip_version"                                                               \
    "\xA1\x04"                                                                 \
    "\x4D"                                                                     \
    "database_type"                                                            \
    "\x44"                                                                     \
    "Test"                                                                     \
    "\x49"                                                                     \
    "languages"                                                                \
    "\x00\x04"                                                                 \
    "\x5B"                                                                     \
    "binary_format_major_version"                                              \
    "\xA1\x02"                                                                 \
    "\x5B"                                                                     \
    "binary_format_minor_version"                                              \
    "\xA0"                                                                     \
    "\x4B"                                                                     \
    "build_epoch"                                                              \
    "\x01\x02\x01"

// The nine metadata keys that MMDB_open requires, without the map header.
#define REQUIRED_METADATA_KEYS                                                 \
    REQUIRED_KEYS_BEFORE_DESCRIPTION                                           \
    "\x4B"                                                                     \
    "description"                                                              \
    "\xE0"

#define TEST_OPEN(db, expected, message)                                       \
    test_open(db, sizeof(db) - 1, expected, message)

static void
test_open(const char *db, size_t size, int expected, const char *message) {
    char filename[] = "/tmp/duplicate_metadata_key_t_XXXXXX";
    int fd = mkstemp(filename);
    if (-1 == fd) {
        BAIL_OUT("could not create a temporary file");
    }
    FILE *fp = fdopen(fd, "wb");
    if (NULL == fp) {
        BAIL_OUT("could not open %s for writing", filename);
    }
    size_t written = fwrite(db, 1, size, fp);
    if (0 != fclose(fp) || written != size) {
        BAIL_OUT("could not write %s", filename);
    }

    MMDB_s mmdb;
    int status = MMDB_open(filename, MMDB_MODE_MMAP, &mmdb);
    cmp_ok(status, "==", expected, message);
    if (MMDB_SUCCESS == status) {
        MMDB_close(&mmdb);
    }

    if (0 != remove(filename)) {
        BAIL_OUT("could not remove %s", filename);
    }
}

int main(void) {
    plan(NO_PLAN);

    TEST_OPEN(DB_PREFIX "\xE9" REQUIRED_METADATA_KEYS,
              MMDB_SUCCESS,
              "MMDB_open accepts metadata without repeated keys");

    TEST_OPEN(DB_PREFIX "\xEA" REQUIRED_METADATA_KEYS "\x45"
                        "extra"
                        "\xA1\x05",
              MMDB_SUCCESS,
              "MMDB_open accepts an extra distinct metadata key");

    TEST_OPEN(DB_PREFIX "\xEA" REQUIRED_METADATA_KEYS "\x4D"
                        "database_type"
                        "\xA1\x05",
              MMDB_INVALID_METADATA_ERROR,
              "MMDB_open rejects a repeated database_type key");

    TEST_OPEN(DB_PREFIX "\xEA" REQUIRED_METADATA_KEYS "\x4B"
                        "description"
                        "\xE0",
              MMDB_INVALID_METADATA_ERROR,
              "MMDB_open rejects a repeated description key");

    TEST_OPEN(DB_PREFIX "\xEA" REQUIRED_METADATA_KEYS "\x20\x29"
                        "\xA1\x05",
              MMDB_INVALID_METADATA_ERROR,
              "MMDB_open rejects a repeated key reached through a pointer");

    TEST_OPEN(DB_PREFIX "\xE9" REQUIRED_KEYS_BEFORE_DESCRIPTION "\x4B"
                        "description"
                        "\xE2"
                        "\x42"
                        "en"
                        "\x41"
                        "A"
                        "\x42"
                        "en"
                        "\x41"
                        "B",
              MMDB_INVALID_METADATA_ERROR,
              "MMDB_open rejects a repeated key in the description map");

    done_testing();
}
