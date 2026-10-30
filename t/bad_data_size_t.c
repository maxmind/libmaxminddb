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

int main(void) {
    plan(NO_PLAN);
    test_bad_data_size_rejected();
    test_bad_map_size_rejected();
    test_truncated_size_in_tiny_section();
    test_truncated_size_in_tiny_metadata_section();
    done_testing();
}
