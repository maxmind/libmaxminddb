#define _XOPEN_SOURCE 700

#include "maxminddb_test_helper.h"

#include <stdbool.h>
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

// Writes db to a new file in TMPDIR, or /tmp, and puts its path in filename.
static void write_temp_db(const char *db,
                          size_t size,
                          char *filename,
                          size_t filename_size) {
    const char *tmpdir = getenv("TMPDIR");
    if (NULL == tmpdir || '\0' == tmpdir[0]) {
        tmpdir = "/tmp";
    }
    int length = snprintf(
        filename, filename_size, "%s/duplicate_metadata_key_t_XXXXXX", tmpdir);
    if (length < 0 || (size_t)length >= filename_size) {
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
}

static void
test_open(const char *db, size_t size, int expected, const char *message) {
    char filename[4096];
    write_temp_db(db, size, filename, sizeof(filename));

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

#ifndef MAXIMUM_DATA_STRUCTURE_DEPTH
    #define MAXIMUM_DATA_STRUCTURE_DEPTH 512
#endif

// An extra key whose value is a chain of `arrays` nested one-element arrays
// around a uint16. The value is at depth 1, and the uint16 is at depth
// arrays + 1.
// MMDB_get_metadata_as_entry_data_list fails when a value reaches
// MAXIMUM_DATA_STRUCTURE_DEPTH, and MMDB_open must agree with it.
static void test_deep_extra_value(size_t arrays, int expected) {
    const char head[] = DB_PREFIX "\xEA" REQUIRED_METADATA_KEYS "\x45"
                                  "extra";
    const size_t head_size = sizeof(head) - 1;
    size_t size = head_size + arrays * 2 + 1;
    char *db = malloc(size);
    if (NULL == db) {
        BAIL_OUT("could not allocate the deep test database");
    }
    memcpy(db, head, head_size);
    for (size_t i = 0; i < arrays; i++) {
        db[head_size + i * 2] = '\x01';
        db[head_size + i * 2 + 1] = '\x04';
    }
    db[size - 1] = '\xA0';

    char filename[4096];
    write_temp_db(db, size, filename, sizeof(filename));
    free(db);

    MMDB_s mmdb;
    int status = MMDB_open(filename, MMDB_MODE_MMAP, &mmdb);
    cmp_ok(status,
           "==",
           expected,
           "MMDB_open status for an extra value with %zu nested arrays",
           arrays);
    if (MMDB_SUCCESS == status) {
        MMDB_entry_data_list_s *list = NULL;
        status = MMDB_get_metadata_as_entry_data_list(&mmdb, &list);
        cmp_ok(status,
               "==",
               MMDB_SUCCESS,
               "MMDB_get_metadata_as_entry_data_list reads the same metadata");
        MMDB_free_entry_data_list(list);
        MMDB_close(&mmdb);
    }

    if (0 != remove(filename)) {
        BAIL_OUT("could not remove %s", filename);
    }
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

// Builds metadata with count distinct keys, either in the top-level map
// (counting the nine required keys) or in the description map, and checks
// the MMDB_open status. count must be at least 285.
static void test_key_count(bool in_description,
                           size_t count,
                           int expected,
                           const char *message) {
    const char required[] = REQUIRED_METADATA_KEYS;
    const char before_description[] =
        REQUIRED_KEYS_BEFORE_DESCRIPTION "\x4B"
                                         "description";
    size_t size = sizeof(DB_PREFIX) + sizeof(required) + 3 + count * 8;
    char *db = malloc(size);
    if (NULL == db) {
        BAIL_OUT("could not allocate the key-count test database");
    }
    size_t pos = sizeof(DB_PREFIX) - 1;
    memcpy(db, DB_PREFIX, pos);
    size_t keys = count;
    if (in_description) {
        db[pos++] = '\xE9';
        memcpy(db + pos, before_description, sizeof(before_description) - 1);
        pos += sizeof(before_description) - 1;
    } else {
        keys = count - 9;
    }
    // A map header with the 2-byte size form, for 285 through 65,820.
    db[pos++] = '\xFE';
    db[pos++] = (char)((count - 285) >> 8);
    db[pos++] = (char)((count - 285) & 0xFF);
    if (!in_description) {
        memcpy(db + pos, required, sizeof(required) - 1);
        pos += sizeof(required) - 1;
    }
    for (size_t i = 0; i < keys; i++) {
        char key[24];
        snprintf(key, sizeof(key), "k%04zu", i);
        db[pos++] = '\x45';
        memcpy(db + pos, key, 5);
        pos += 5;
        db[pos++] = '\x41';
        db[pos++] = 'x';
    }

    test_open(db, pos, expected, message);
    free(db);
}

// A record {"a": {["x"]: "y"}, "b": "z"}. Both search-tree records point to
// it. Looking up "b" skips the value of "a", which has an array key.
static void test_lookup_after_array_key(void) {
    const char db[] = "\x00\x00\x11\x00\x00\x11"
                      "\x00\x00\x00\x00\x00\x00\x00\x00"
                      "\x00\x00\x00\x00\x00\x00\x00\x00"
                      "\xE2"
                      "\x41"
                      "a"
                      "\xE1"
                      "\x01\x04"
                      "\x41"
                      "x"
                      "\x41"
                      "y"
                      "\x41"
                      "b"
                      "\x41"
                      "z"
                      "\xAB\xCD\xEF"
                      "MaxMind.com"
                      "\xE9" REQUIRED_METADATA_KEYS;
    char filename[4096];
    write_temp_db(db, sizeof(db) - 1, filename, sizeof(filename));

    MMDB_s mmdb;
    int status = MMDB_open(filename, MMDB_MODE_MMAP, &mmdb);
    cmp_ok(status, "==", MMDB_SUCCESS, "MMDB_open opens the array-key record");
    if (MMDB_SUCCESS == status) {
        int gai_error, mmdb_error;
        MMDB_lookup_result_s result =
            MMDB_lookup_string(&mmdb, "1.1.1.1", &gai_error, &mmdb_error);
        cmp_ok(mmdb_error, "==", MMDB_SUCCESS, "lookup succeeded");
        ok(result.found_entry, "lookup found the record");
        MMDB_entry_data_s data;
        status = MMDB_get_value(&result.entry, &data, "b", NULL);
        cmp_ok(status,
               "==",
               MMDB_INVALID_DATA_ERROR,
               "MMDB_get_value rejects a nested map key that is an array");
        MMDB_close(&mmdb);
    }

    if (0 != remove(filename)) {
        BAIL_OUT("could not remove %s", filename);
    }
}

// Major version 3 metadata with an extra value that this version cannot
// decode. MMDB_open must report the unknown format, not the bad value.
static void test_newer_major_version(void) {
    char db[] = DB_PREFIX "\xEA" REQUIRED_METADATA_KEYS "\x45"
                          "extra"
                          "\x44"
                          "ab";
    const char key[] = "binary_format_major_version";
    size_t size = sizeof(db) - 1;
    size_t version = 0;
    for (size_t i = 0; i + sizeof(key) + 1 < size; i++) {
        if (0 == memcmp(db + i, key, sizeof(key) - 1)) {
            version = i + sizeof(key) - 1 + 1;
            break;
        }
    }
    if (0 == version || '\x02' != db[version]) {
        BAIL_OUT("could not find binary_format_major_version in the fixture");
    }
    db[version] = '\x03';

    test_open(db,
              size,
              MMDB_UNKNOWN_DATABASE_FORMAT_ERROR,
              "MMDB_open reports a newer major version before bad metadata");
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

    test_deep_extra_value(MAXIMUM_DATA_STRUCTURE_DEPTH - 2, MMDB_SUCCESS);
    test_deep_extra_value(MAXIMUM_DATA_STRUCTURE_DEPTH - 1,
                          MMDB_INVALID_METADATA_ERROR);
    test_long_pointer_keys();
    test_key_count(false,
                   1024,
                   MMDB_SUCCESS,
                   "MMDB_open accepts 1,024 top-level metadata keys");
    test_key_count(false,
                   1025,
                   MMDB_INVALID_METADATA_ERROR,
                   "MMDB_open rejects 1,025 top-level metadata keys");
    test_key_count(true,
                   1024,
                   MMDB_SUCCESS,
                   "MMDB_open accepts 1,024 description languages");
    test_key_count(true,
                   1025,
                   MMDB_INVALID_METADATA_ERROR,
                   "MMDB_open rejects 1,025 description languages");

    test_lookup_after_array_key();
    test_newer_major_version();

    done_testing();
}
