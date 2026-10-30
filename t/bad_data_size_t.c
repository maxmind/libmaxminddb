#define _XOPEN_SOURCE 700

#include "maxminddb_test_helper.h"

#include <unistd.h>

// One IPv4 node with 24-bit records. The left record points to data section
// offset 0. The right record is empty.
#define TREE_LEFT_DATA "\x00\x00\x11\x00\x00\x01"

// Both records are empty.
#define TREE_EMPTY "\x00\x00\x01\x00\x00\x01"

// The right record points to data section offset 3. The left record is empty.
#define TREE_RIGHT_OFFSET_3 "\x00\x00\x01\x00\x00\x14"

#define SEPARATOR                                                              \
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"

#define METADATA_MARKER                                                        \
    "\xAB\xCD\xEF"                                                             \
    "MaxMind.com"

// A map with the nine metadata keys that MMDB_open requires.
#define METADATA                                                               \
    "\xE9"                                                                     \
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
    "\x01\x02\x01"                                                             \
    "\x4B"                                                                     \
    "description"                                                              \
    "\xE0"

#define CHECK_ENTRY(tree, data, ip, lookup_status, decode_status, message)     \
    check_entry(tree SEPARATOR data METADATA_MARKER METADATA,                  \
                sizeof(tree SEPARATOR data METADATA_MARKER METADATA) - 1,      \
                sizeof(data) - 1,                                              \
                ip,                                                            \
                lookup_status,                                                 \
                decode_status,                                                 \
                message)

// Writes db to a temporary file, opens it, and removes the file.
static int open_db(const char *db, size_t db_size, MMDB_s *mmdb) {
    char path[] = "/tmp/bad_data_size_t.XXXXXX";
    int fd = mkstemp(path);
    if (-1 == fd) {
        BAIL_OUT("could not create %s", path);
    }
    FILE *fp = fdopen(fd, "wb");
    if (NULL == fp) {
        close(fd);
        unlink(path);
        BAIL_OUT("could not open %s", path);
    }
    size_t written = fwrite(db, 1, db_size, fp);
    if (0 != fclose(fp) || written != db_size) {
        unlink(path);
        BAIL_OUT("could not write %s", path);
    }

    int status = MMDB_open(path, MMDB_MODE_MMAP, mmdb);
    if (0 != unlink(path)) {
        BAIL_OUT("could not remove %s", path);
    }
    return status;
}

static void check_entry(const char *db,
                        size_t db_size,
                        size_t data_size,
                        const char *ip,
                        int lookup_status,
                        int decode_status,
                        const char *message) {
    MMDB_s mmdb;
    int status = open_db(db, db_size, &mmdb);
    cmp_ok(status, "==", MMDB_SUCCESS, "opened MMDB: %s", message);
    if (MMDB_SUCCESS != status) {
        return;
    }
    cmp_ok((int)mmdb.data_section_size,
           "==",
           (int)data_size,
           "data section ends at the metadata marker: %s",
           message);

    int gai_error, mmdb_error;
    MMDB_lookup_result_s result =
        MMDB_lookup_string(&mmdb, ip, &gai_error, &mmdb_error);
    cmp_ok(mmdb_error, "==", lookup_status, "lookup status: %s", message);

    if (MMDB_SUCCESS == mmdb_error) {
        ok(result.found_entry, "entry found: %s", message);
        MMDB_entry_data_list_s *entry_data_list = NULL;
        status = MMDB_get_entry_data_list(&result.entry, &entry_data_list);
        cmp_ok(status, "==", decode_status, "decode status: %s", message);
        MMDB_free_entry_data_list(entry_data_list);
    }
    MMDB_close(&mmdb);
}

// The section is a heap buffer of the exact size, so a sanitizer reports any
// read past its end.
static void check_tiny_section(const char *section,
                               uint32_t section_size,
                               const char *message) {
    uint8_t *data = malloc(section_size);
    if (NULL == data) {
        BAIL_OUT("could not allocate section");
    }
    memcpy(data, section, section_size);

    MMDB_s mmdb = {.data_section = data, .data_section_size = section_size};
    MMDB_entry_s entry = {.mmdb = &mmdb, .offset = 0};
    MMDB_entry_data_list_s *entry_data_list = NULL;
    int status = MMDB_get_entry_data_list(&entry, &entry_data_list);
    cmp_ok(status, "==", MMDB_INVALID_DATA_ERROR, "%s", message);
    MMDB_free_entry_data_list(entry_data_list);
    free(data);
}

void test_truncated_size_in_tiny_section(void) {
    // A size of 30 needs two more size bytes.
    check_tiny_section("\x5E", 1, "truncated size in a 1-byte section");
    // A size of 31 needs three more size bytes.
    check_tiny_section("\x5F\x00", 2, "truncated size in a 2-byte section");
}

// An extended boolean with a size of 31 needs three more size bytes. A
// boolean has no payload, so no later check catches the bad size.
#define TRUNCATED_BOOLEAN "\x1F\x07"

void test_truncated_size_in_tiny_metadata_section(void) {
    MMDB_s mmdb;
    int status = open_db(METADATA_MARKER TRUNCATED_BOOLEAN,
                         sizeof(METADATA_MARKER TRUNCATED_BOOLEAN) - 1,
                         &mmdb);
    cmp_ok(status,
           "==",
           MMDB_INVALID_DATA_ERROR,
           "truncated size in a 2-byte metadata section");
    if (MMDB_SUCCESS == status) {
        MMDB_close(&mmdb);
    }
}

void test_tiny_data_section(void) {
    CHECK_ENTRY(TREE_LEFT_DATA,
                "\xE0",
                "1.2.3.4",
                MMDB_SUCCESS,
                MMDB_SUCCESS,
                "empty map in a 1-byte data section");

    CHECK_ENTRY(TREE_LEFT_DATA,
                "\x41"
                "a",
                "1.2.3.4",
                MMDB_SUCCESS,
                MMDB_SUCCESS,
                "string in a 2-byte data section");

    CHECK_ENTRY(TREE_LEFT_DATA,
                TRUNCATED_BOOLEAN,
                "1.2.3.4",
                MMDB_SUCCESS,
                MMDB_INVALID_DATA_ERROR,
                "truncated size in a 2-byte data section");
}

void test_data_section_bounded_at_metadata_marker(void) {
    CHECK_ENTRY(TREE_LEFT_DATA,
                "\x42"
                "ab",
                "1.2.3.4",
                MMDB_SUCCESS,
                MMDB_SUCCESS,
                "string that ends at the metadata marker");

    CHECK_ENTRY(TREE_LEFT_DATA,
                "\x43"
                "ab",
                "1.2.3.4",
                MMDB_SUCCESS,
                MMDB_INVALID_DATA_ERROR,
                "string that extends into the metadata marker");

    // The pointer targets data section offset 17, the start of the metadata.
    CHECK_ENTRY(TREE_LEFT_DATA,
                "\x20\x11\x00",
                "1.2.3.4",
                MMDB_SUCCESS,
                MMDB_INVALID_DATA_ERROR,
                "pointer into the metadata");

    CHECK_ENTRY(TREE_RIGHT_OFFSET_3,
                "\x42"
                "ab",
                "128.0.0.1",
                MMDB_CORRUPT_SEARCH_TREE_ERROR,
                MMDB_SUCCESS,
                "search tree record that points at the metadata marker");

    // The data section holds a string that contains a metadata marker. Only
    // the last marker starts the metadata.
    CHECK_ENTRY(TREE_LEFT_DATA,
                "\x4E" METADATA_MARKER,
                "1.2.3.4",
                MMDB_SUCCESS,
                MMDB_SUCCESS,
                "string that contains a metadata marker");
}

void test_empty_data_section(void) {
    const char db[] = TREE_EMPTY SEPARATOR METADATA_MARKER METADATA;
    MMDB_s mmdb;
    int status = open_db(db, sizeof(db) - 1, &mmdb);
    cmp_ok(status, "==", MMDB_SUCCESS, "opened MMDB with empty data section");
    if (MMDB_SUCCESS == status) {
        cmp_ok((int)mmdb.data_section_size, "==", 0, "data section is empty");

        int gai_error, mmdb_error;
        MMDB_lookup_result_s result =
            MMDB_lookup_string(&mmdb, "1.2.3.4", &gai_error, &mmdb_error);
        cmp_ok(mmdb_error, "==", MMDB_SUCCESS, "lookup in empty data section");
        ok(!result.found_entry, "no entry in empty data section");
        MMDB_close(&mmdb);
    }
}

void test_bad_data_size_rejected(void) {
    char *db_file = bad_database_path("libmaxminddb-oversized-array.mmdb");

    MMDB_s mmdb;
    int status = MMDB_open(db_file, MMDB_MODE_MMAP, &mmdb);
    cmp_ok(status, "==", MMDB_SUCCESS, "opened bad-data-size MMDB");

    if (status != MMDB_SUCCESS) {
        diag("MMDB_open failed: %s", MMDB_strerror(status));
        free(db_file);
        return;
    }

    int gai_error, mmdb_error;
    MMDB_lookup_result_s result =
        MMDB_lookup_string(&mmdb, "1.2.3.4", &gai_error, &mmdb_error);

    cmp_ok(mmdb_error, "==", MMDB_SUCCESS, "lookup succeeded");
    ok(result.found_entry, "entry found");

    if (result.found_entry) {
        MMDB_entry_data_list_s *entry_data_list = NULL;
        status = MMDB_get_entry_data_list(&result.entry, &entry_data_list);
        cmp_ok(status,
               "==",
               MMDB_INVALID_DATA_ERROR,
               "MMDB_get_entry_data_list returns INVALID_DATA_ERROR "
               "for array with size exceeding remaining data");
        MMDB_free_entry_data_list(entry_data_list);
    }

    MMDB_close(&mmdb);
    free(db_file);
}

void test_bad_map_size_rejected(void) {
    char *db_file = bad_database_path("libmaxminddb-oversized-map.mmdb");

    MMDB_s mmdb;
    int status = MMDB_open(db_file, MMDB_MODE_MMAP, &mmdb);
    cmp_ok(status, "==", MMDB_SUCCESS, "opened bad-map-size MMDB");

    if (status != MMDB_SUCCESS) {
        diag("MMDB_open failed: %s", MMDB_strerror(status));
        free(db_file);
        return;
    }

    int gai_error, mmdb_error;
    MMDB_lookup_result_s result =
        MMDB_lookup_string(&mmdb, "1.2.3.4", &gai_error, &mmdb_error);

    cmp_ok(mmdb_error, "==", MMDB_SUCCESS, "lookup succeeded");
    ok(result.found_entry, "entry found");

    if (result.found_entry) {
        MMDB_entry_data_list_s *entry_data_list = NULL;
        status = MMDB_get_entry_data_list(&result.entry, &entry_data_list);
        cmp_ok(status,
               "==",
               MMDB_INVALID_DATA_ERROR,
               "MMDB_get_entry_data_list returns INVALID_DATA_ERROR "
               "for map with size exceeding remaining data");
        MMDB_free_entry_data_list(entry_data_list);
    }

    MMDB_close(&mmdb);
    free(db_file);
}

int main(void) {
    plan(NO_PLAN);
    test_bad_data_size_rejected();
    test_bad_map_size_rejected();
    test_truncated_size_in_tiny_section();
    test_truncated_size_in_tiny_metadata_section();
    test_tiny_data_section();
    test_data_section_bounded_at_metadata_marker();
    test_empty_data_section();
    done_testing();
}
