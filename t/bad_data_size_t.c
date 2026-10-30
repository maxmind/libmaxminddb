#define _XOPEN_SOURCE 700

#include "maxminddb_test_helper.h"

#include <unistd.h>

#define METADATA_MARKER                                                        \
    "\xAB\xCD\xEF"                                                             \
    "MaxMind.com"

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
    done_testing();
}
