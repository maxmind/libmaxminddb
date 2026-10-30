#include "maxminddb_test_helper.h"

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

// The section is a heap buffer of the exact size, so a sanitizer reports any
// read past its end. A file cannot test this, because a read past the end of
// a mapped file stays inside the last page.
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
    cmp_ok(status, "==", MMDB_INVALID_DATA_ERROR, "data list: %s", message);
    MMDB_free_entry_data_list(entry_data_list);

    MMDB_entry_data_s entry_data;
    const char *path[] = {NULL};
    status = MMDB_aget_value(&entry, &entry_data, path);
    cmp_ok(status, "==", MMDB_INVALID_DATA_ERROR, "value: %s", message);
    free(data);
}

void test_truncated_size_in_tiny_section(void) {
    // A size of 30 needs two more size bytes.
    check_tiny_section("\x5E", 1, "truncated size in a 1-byte section");
    check_tiny_section("\xFE", 1, "truncated map size in a 1-byte section");
    // A size of 31 needs three more size bytes.
    check_tiny_section("\x5F\x00", 2, "truncated size in a 2-byte section");
    check_tiny_section("\xFF\x00", 2, "truncated map size in a 2-byte section");
}

static void check_open_fails(const char *filename, int open_status) {
    char *db_file = bad_database_path(filename);
    MMDB_s mmdb;
    int status = MMDB_open(db_file, MMDB_MODE_MMAP, &mmdb);
    cmp_ok(status, "==", open_status, "open status: %s", filename);
    if (MMDB_SUCCESS == status) {
        MMDB_close(&mmdb);
    }
    free(db_file);
}

void test_truncated_size_in_tiny_metadata_section(void) {
    check_open_fails("libmaxminddb-metadata-truncated-size.mmdb",
                     MMDB_INVALID_DATA_ERROR);
}

// Opens a database with a 1-node search tree, checks the size of its data
// section, and looks up ip. When the lookup finds an entry, decode_status is
// the expected result of decoding it.
static void check_entry(const char *filename,
                        uint32_t data_size,
                        const char *ip,
                        int lookup_status,
                        int decode_status) {
    char *db_file = bad_database_path(filename);
    MMDB_s mmdb;
    int status = MMDB_open(db_file, MMDB_MODE_MMAP, &mmdb);
    free(db_file);
    cmp_ok(status, "==", MMDB_SUCCESS, "opened MMDB: %s", filename);
    if (MMDB_SUCCESS != status) {
        return;
    }
    cmp_ok(mmdb.data_section_size,
           "==",
           data_size,
           "data section ends at the metadata marker: %s",
           filename);

    MMDB_search_node_s node;
    status = MMDB_read_node(&mmdb, 0, &node);
    cmp_ok(status, "==", lookup_status, "read node status: %s", filename);

    int gai_error, mmdb_error;
    MMDB_lookup_result_s result =
        MMDB_lookup_string(&mmdb, ip, &gai_error, &mmdb_error);
    cmp_ok(gai_error, "==", 0, "lookup string parse: %s", filename);
    cmp_ok(mmdb_error, "==", lookup_status, "lookup status: %s", filename);

    if (MMDB_SUCCESS == mmdb_error) {
        ok(result.found_entry, "entry found: %s", filename);
        MMDB_entry_data_list_s *entry_data_list = NULL;
        status = MMDB_get_entry_data_list(&result.entry, &entry_data_list);
        cmp_ok(status, "==", decode_status, "decode status: %s", filename);
        MMDB_free_entry_data_list(entry_data_list);
    } else {
        ok(!result.found_entry, "no entry found: %s", filename);
    }
    MMDB_close(&mmdb);
}

void test_tiny_data_section(void) {
    check_entry("libmaxminddb-data-section-1-byte.mmdb",
                1,
                "1.2.3.4",
                MMDB_SUCCESS,
                MMDB_SUCCESS);
    check_entry("libmaxminddb-data-section-2-bytes.mmdb",
                2,
                "1.2.3.4",
                MMDB_SUCCESS,
                MMDB_SUCCESS);
    check_entry("libmaxminddb-data-section-truncated-size.mmdb",
                2,
                "1.2.3.4",
                MMDB_SUCCESS,
                MMDB_INVALID_DATA_ERROR);
}

void test_data_section_bounded_at_metadata_marker(void) {
    check_entry("libmaxminddb-data-ends-at-metadata-marker.mmdb",
                3,
                "1.2.3.4",
                MMDB_SUCCESS,
                MMDB_SUCCESS);
    check_entry("libmaxminddb-data-extends-into-metadata-marker.mmdb",
                3,
                "1.2.3.4",
                MMDB_SUCCESS,
                MMDB_INVALID_DATA_ERROR);
    check_entry("libmaxminddb-pointer-into-metadata.mmdb",
                3,
                "1.2.3.4",
                MMDB_SUCCESS,
                MMDB_INVALID_DATA_ERROR);
    // The right record points at the metadata marker.
    check_entry("libmaxminddb-record-points-at-metadata-marker.mmdb",
                3,
                "128.0.0.1",
                MMDB_CORRUPT_SEARCH_TREE_ERROR,
                MMDB_SUCCESS);
    // The data is a string that contains a metadata marker. Only the last
    // marker starts the metadata.
    check_entry("libmaxminddb-data-contains-metadata-marker.mmdb",
                15,
                "1.2.3.4",
                MMDB_SUCCESS,
                MMDB_SUCCESS);
}

void test_empty_data_section(void) {
    char *db_file = bad_database_path("libmaxminddb-data-section-empty.mmdb");
    MMDB_s mmdb;
    int status = MMDB_open(db_file, MMDB_MODE_MMAP, &mmdb);
    free(db_file);
    cmp_ok(status, "==", MMDB_SUCCESS, "opened MMDB with empty data section");
    if (MMDB_SUCCESS != status) {
        return;
    }
    cmp_ok(mmdb.data_section_size, "==", 0, "data section is empty");

    int gai_error, mmdb_error;
    MMDB_lookup_result_s result =
        MMDB_lookup_string(&mmdb, "1.2.3.4", &gai_error, &mmdb_error);
    cmp_ok(gai_error, "==", 0, "lookup string parse succeeds");
    cmp_ok(mmdb_error, "==", MMDB_SUCCESS, "lookup in empty data section");
    ok(!result.found_entry, "no entry in empty data section");
    MMDB_close(&mmdb);
}

void test_search_tree_overlaps_metadata_marker(void) {
    check_open_fails("libmaxminddb-no-search-tree.mmdb",
                     MMDB_INVALID_METADATA_ERROR);
    check_open_fails("libmaxminddb-no-data-separator.mmdb",
                     MMDB_INVALID_METADATA_ERROR);
    check_open_fails("libmaxminddb-short-data-separator.mmdb",
                     MMDB_INVALID_METADATA_ERROR);
}

void test_node_count_larger_than_file(void) {
    char *db_file =
        test_database_path("GeoIP2-City-Test-Invalid-Node-Count.mmdb");
    MMDB_s mmdb;
    int status = MMDB_open(db_file, MMDB_MODE_MMAP, &mmdb);
    cmp_ok(status,
           "==",
           MMDB_INVALID_METADATA_ERROR,
           "node count larger than the file");
    if (MMDB_SUCCESS == status) {
        MMDB_close(&mmdb);
    }
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
    test_search_tree_overlaps_metadata_marker();
    test_node_count_larger_than_file();
    done_testing();
}
