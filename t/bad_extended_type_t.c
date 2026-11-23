#include "maxminddb_test_helper.h"

// Copies section into a heap buffer of the exact size, so a sanitizer reports
// any read past its end.
static uint8_t *make_section(const char *section, size_t size) {
    uint8_t *data = malloc(size);
    if (NULL == data) {
        BAIL_OUT("could not allocate section");
    }
    memcpy(data, section, size);
    return data;
}

static void check_entry_data_list(const char *section,
                                  size_t size,
                                  int expected_status,
                                  int expected_type,
                                  const char *message) {
    uint8_t *data = make_section(section, size);
    MMDB_s mmdb = {.data_section = data, .data_section_size = (uint32_t)size};
    MMDB_entry_s entry = {.mmdb = &mmdb, .offset = 0};
    MMDB_entry_data_list_s *entry_data_list = NULL;

    int status = MMDB_get_entry_data_list(&entry, &entry_data_list);
    cmp_ok(status, "==", expected_status, "status: %s", message);
    if (MMDB_SUCCESS == expected_status && MMDB_SUCCESS == status) {
        cmp_ok(entry_data_list->entry_data.type,
               "==",
               expected_type,
               "type: %s",
               message);
    }

    MMDB_free_entry_data_list(entry_data_list);
    free(data);
}

#define CHECK_ENTRY_DATA_LIST(section, status, type, message)                  \
    check_entry_data_list(section, sizeof(section) - 1, status, type, message)

void test_extended_type_range(void) {
    // The control byte 0x00 is an extended type with a size of 0. The
    // MaxMind-DB databases cover the rejected bytes.
    CHECK_ENTRY_DATA_LIST("\x20\x02\x00\x09",
                          MMDB_INVALID_DATA_ERROR,
                          0,
                          "a pointer to extended type byte 9 is rejected");

    CHECK_ENTRY_DATA_LIST("\x00\x01",
                          MMDB_SUCCESS,
                          MMDB_DATA_TYPE_INT32,
                          "extended type byte 1 is an int32");
    CHECK_ENTRY_DATA_LIST("\x00\x04",
                          MMDB_SUCCESS,
                          MMDB_DATA_TYPE_ARRAY,
                          "extended type byte 4 is an array");
    CHECK_ENTRY_DATA_LIST("\x00\x07",
                          MMDB_SUCCESS,
                          MMDB_DATA_TYPE_BOOLEAN,
                          "extended type byte 7 is a boolean");
    CHECK_ENTRY_DATA_LIST("\x04\x08\x00\x00\x00\x00",
                          MMDB_SUCCESS,
                          MMDB_DATA_TYPE_FLOAT,
                          "extended type byte 8 is a float");
}

// An array of [<bad extended type byte>, 5]. MMDB_get_value skips the first
// element on its way to index 1.
void test_extended_type_in_skipped_value(void) {
    const char section[] = "\x02\x04"
                           "\x00\x09"
                           "\xA1\x05";
    uint8_t *data = make_section(section, sizeof(section) - 1);
    MMDB_s mmdb = {.data_section = data,
                   .data_section_size = (uint32_t)(sizeof(section) - 1)};
    MMDB_entry_s entry = {.mmdb = &mmdb, .offset = 0};
    MMDB_entry_data_s entry_data;

    int status = MMDB_get_value(&entry, &entry_data, "1", NULL);
    cmp_ok(status,
           "==",
           MMDB_INVALID_DATA_ERROR,
           "a skipped array element with extended type byte 9 is rejected");

    free(data);
}

// The MaxMind-DB databases whose data record is
// {"ip": <bad value>, "after": "test"}.
static void test_bad_data_files(void) {
    const char *const filenames[] = {
        "libmaxminddb-extended-type-0.mmdb",
        "libmaxminddb-extended-type-9.mmdb",
        "libmaxminddb-container-type.mmdb",
        "libmaxminddb-end-marker-type.mmdb",
    };
    for (size_t i = 0; i < sizeof(filenames) / sizeof(filenames[0]); i++) {
        const char *filename = filenames[i];
        char *path = bad_database_path(filename);
        MMDB_s mmdb;
        int status = MMDB_open(path, MMDB_MODE_MMAP, &mmdb);
        free(path);
        cmp_ok(status, "==", MMDB_SUCCESS, "MMDB_open opens %s", filename);
        if (MMDB_SUCCESS != status) {
            continue;
        }

        int gai_error, mmdb_error;
        MMDB_lookup_result_s result =
            MMDB_lookup_string(&mmdb, "1.1.1.1", &gai_error, &mmdb_error);
        ok(0 == gai_error && MMDB_SUCCESS == mmdb_error && result.found_entry,
           "lookup found the record in %s",
           filename);

        MMDB_entry_data_list_s *entry_data_list = NULL;
        status = MMDB_get_entry_data_list(&result.entry, &entry_data_list);
        cmp_ok(status,
               "==",
               MMDB_INVALID_DATA_ERROR,
               "MMDB_get_entry_data_list rejects the record in %s",
               filename);
        MMDB_free_entry_data_list(entry_data_list);

        MMDB_entry_data_s entry_data;
        status = MMDB_get_value(&result.entry, &entry_data, "ip", NULL);
        cmp_ok(status,
               "==",
               MMDB_INVALID_DATA_ERROR,
               "MMDB_get_value rejects the value in %s",
               filename);

        // Byte 0 is an empty map if it is not rejected, so a skip would
        // silently succeed.
        status = MMDB_get_value(&result.entry, &entry_data, "after", NULL);
        cmp_ok(status,
               "==",
               MMDB_INVALID_DATA_ERROR,
               "MMDB_get_value rejects the skipped value in %s",
               filename);
        MMDB_close(&mmdb);
    }
}

int main(void) {
    plan(NO_PLAN);
    test_extended_type_range();
    test_extended_type_in_skipped_value();
    test_bad_data_files();
    done_testing();
}
