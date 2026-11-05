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
    const char *tmpdir = getenv("TMPDIR");
    if (NULL == tmpdir || '\0' == tmpdir[0]) {
        tmpdir = "/tmp";
    }
    char filename[4096];
    int length = snprintf(filename,
                          sizeof(filename),
                          "%s/duplicate_metadata_key_t_XXXXXX",
                          tmpdir);
    if (length < 0 || (size_t)length >= sizeof(filename)) {
        BAIL_OUT("TMPDIR is too long");
    }
    int fd = mkstemp(filename);
    if (-1 == fd) {
        BAIL_OUT("could not create a temporary file in %s", tmpdir);
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

// An extra key whose value is 600 nested one-element arrays, deeper than the
// default MAXIMUM_DATA_STRUCTURE_DEPTH of 512.
static void test_deep_extra_value(void) {
    const char head[] = DB_PREFIX "\xEA" REQUIRED_METADATA_KEYS "\x45"
                                  "extra";
    const size_t head_size = sizeof(head) - 1;
    const size_t depth = 600;
    size_t size = head_size + depth * 2 + 1;
    char *db = malloc(size);
    if (NULL == db) {
        BAIL_OUT("could not allocate the deep test database");
    }
    memcpy(db, head, head_size);
    for (size_t i = 0; i < depth; i++) {
        db[head_size + i * 2] = '\x01';
        db[head_size + i * 2 + 1] = '\x04';
    }
    db[size - 1] = '\xA0';

    test_open(db,
              size,
              MMDB_INVALID_METADATA_ERROR,
              "MMDB_open rejects an extra value nested past the depth limit");
    free(db);
}

// Pointer keys into a run of 0x5E bytes. The key at run offset j is a
// 24,443-byte string that starts with 100 - j bytes of 0x5E, so the keys are
// distinct, but together they exceed the 2 MiB payload budget.
static void test_long_pointer_keys(void) {
    const size_t key_count = 100;
    const size_t run = key_count + 3;
    const size_t key_size = 285 + 0x5E5E;
    const size_t blob_size = run + key_size;
    const char head[] = DB_PREFIX "\xFD\x51" REQUIRED_METADATA_KEYS "\x44"
                                  "blob"
                                  "\x5E";
    const size_t head_size = sizeof(head) - 1;
    const size_t prefix_size = sizeof(DB_PREFIX) - 1;
    // The metadata offset of the blob content, after its 3-byte header.
    const size_t blob_offset = head_size + 2 - prefix_size;

    size_t size = head_size + 2 + blob_size + key_count * 3;
    char *db = malloc(size);
    if (NULL == db) {
        BAIL_OUT("could not allocate the long-key test database");
    }
    memcpy(db, head, head_size);
    size_t pos = head_size;
    db[pos++] = (char)((blob_size - 285) >> 8);
    db[pos++] = (char)((blob_size - 285) & 0xFF);
    memset(db + pos, 0x5E, run);
    memset(db + pos + run, 'a', key_size);
    pos += blob_size;
    for (size_t j = 0; j < key_count; j++) {
        size_t target = blob_offset + j;
        db[pos++] = (char)(0x20 | (target >> 8));
        db[pos++] = (char)(target & 0xFF);
        db[pos++] = '\xA0';
    }

    test_open(db,
              size,
              MMDB_INVALID_METADATA_ERROR,
              "MMDB_open rejects metadata keys past the payload budget");
    free(db);
}

static void test_bad_data_file(const char *filename) {
    char *path = bad_database_path(filename);
    MMDB_s mmdb;
    int status = MMDB_open(path, MMDB_MODE_MMAP, &mmdb);
    cmp_ok(status,
           "==",
           MMDB_INVALID_METADATA_ERROR,
           "MMDB_open rejects %s",
           filename);
    if (MMDB_SUCCESS == status) {
        MMDB_close(&mmdb);
    }
    free(path);
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

    // The key points at the database_type value, "Test", at offset 0x37.
    TEST_OPEN(DB_PREFIX "\xEA" REQUIRED_METADATA_KEYS "\x20\x37"
                        "\xA1\x05",
              MMDB_SUCCESS,
              "MMDB_open accepts a distinct key reached through a pointer");

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

    TEST_OPEN(DB_PREFIX "\xEA" REQUIRED_METADATA_KEYS "\x4F"
                        "database_type\x00"
                        "x"
                        "\x44"
                        "Evil",
              MMDB_INVALID_METADATA_ERROR,
              "MMDB_open rejects top-level keys that differ only after a NUL");

    TEST_OPEN(DB_PREFIX "\xE9" REQUIRED_KEYS_BEFORE_DESCRIPTION "\x4B"
                        "description"
                        "\xE2"
                        "\x42"
                        "en"
                        "\x41"
                        "A"
                        "\x44"
                        "en\x00"
                        "x"
                        "\x41"
                        "B",
              MMDB_INVALID_METADATA_ERROR,
              "MMDB_open rejects description keys that differ only after a "
              "NUL");

    TEST_OPEN(DB_PREFIX "\xE9" REQUIRED_KEYS_BEFORE_DESCRIPTION "\x4B"
                        "description"
                        "\xE2"
                        "\x42"
                        "en"
                        "\x41"
                        "A"
                        "\xA1\x01"
                        "\x41"
                        "B",
              MMDB_INVALID_METADATA_ERROR,
              "MMDB_open rejects a non-string key in the description map");

    TEST_OPEN(DB_PREFIX "\xEB" REQUIRED_METADATA_KEYS,
              MMDB_INVALID_DATA_ERROR,
              "MMDB_open rejects a metadata map count larger than its entries");

    TEST_OPEN(DB_PREFIX "\xFE\x00\x00" REQUIRED_METADATA_KEYS,
              MMDB_INVALID_DATA_ERROR,
              "MMDB_open rejects a metadata map count larger than the section");

    TEST_OPEN(DB_PREFIX "\xA1\x05",
              MMDB_INVALID_METADATA_ERROR,
              "MMDB_open rejects metadata whose root is not a map");

    TEST_OPEN(DB_PREFIX "\xEA" REQUIRED_METADATA_KEYS "\xA1\x05"
                        "\xA1\x05",
              MMDB_INVALID_DATA_ERROR,
              "MMDB_open rejects a non-string key after the required keys");

    TEST_OPEN(DB_PREFIX "\xEA" REQUIRED_METADATA_KEYS "\x45"
                        "extra"
                        "\x44"
                        "ab",
              MMDB_INVALID_DATA_ERROR,
              "MMDB_open rejects a truncated value after the required keys");

    // The extra value is {["x"]: "fake"}. Skipping only the header of the
    // array key would make the walk read "fake" as a top-level key and miss
    // the repeated database_type.
    TEST_OPEN(DB_PREFIX "\xEB" REQUIRED_METADATA_KEYS "\x45"
                        "extra"
                        "\xE1"
                        "\x01\x04"
                        "\x41"
                        "x"
                        "\x44"
                        "fake"
                        "\x4D"
                        "database_type"
                        "\x44"
                        "Evil",
              MMDB_INVALID_DATA_ERROR,
              "MMDB_open rejects a nested map key that is an array");

    test_deep_extra_value();
    test_long_pointer_keys();

    test_bad_data_file("libmaxminddb-repeated-metadata-key.mmdb");
    test_bad_data_file("libmaxminddb-repeated-description-key.mmdb");

    done_testing();
}
