#include "maxminddb_test_helper.h"

static void check_entry_data_list(const char *section,
                                  size_t size,
                                  int expected_status,
                                  const char *message) {
    // An exact-size heap buffer lets a sanitizer report any read past its end.
    uint8_t *data = malloc(size);
    if (NULL == data) {
        BAIL_OUT("could not allocate section");
    }
    memcpy(data, section, size);

    MMDB_s mmdb = {.data_section = data, .data_section_size = (uint32_t)size};
    MMDB_entry_s entry = {.mmdb = &mmdb, .offset = 0};
    MMDB_entry_data_list_s *entry_data_list = NULL;

    int status = MMDB_get_entry_data_list(&entry, &entry_data_list);
    cmp_ok(status, "==", expected_status, "status: %s", message);
    if (MMDB_SUCCESS == status) {
        MMDB_entry_data_list_s *key = entry_data_list->next;
        if (cmp_ok(key->entry_data.type,
                   "==",
                   MMDB_DATA_TYPE_UTF8_STRING,
                   "key type: %s",
                   message)) {
            ok(key->entry_data.data_size == 1 &&
                   key->entry_data.utf8_string[0] == 'k',
               "key value: %s",
               message);
        }
    } else {
        ok(NULL == entry_data_list, "list is NULL: %s", message);
    }

    MMDB_free_entry_data_list(entry_data_list);
    free(data);
}

#define CHECK_ENTRY_DATA_LIST(section, status, message)                        \
    check_entry_data_list(section, sizeof(section) - 1, status, message)

static void check_metadata_as_entry_data_list(void) {
    // {1: "x"}, where the key is a uint16.
    uint8_t metadata[] = {0xE1, 0xA1, 0x01, 0x41, 'x'};
    MMDB_s mmdb = {.metadata_section = metadata,
                   .metadata_section_size = sizeof(metadata)};
    MMDB_entry_data_list_s *entry_data_list = NULL;

    int status = MMDB_get_metadata_as_entry_data_list(&mmdb, &entry_data_list);
    cmp_ok(status,
           "==",
           MMDB_INVALID_DATA_ERROR,
           "MMDB_get_metadata_as_entry_data_list rejects a uint16 key");
    ok(NULL == entry_data_list, "the metadata list is NULL");
    MMDB_free_entry_data_list(entry_data_list);
}

// The non-string-key databases from MaxMind-DB.
static void check_bad_data_files(void) {
    // The data record is {1: "test"}, where the key is a uint16.
    char *path = bad_database_path("libmaxminddb-non-string-map-key.mmdb");
    MMDB_s mmdb;
    int status = MMDB_open(path, MMDB_MODE_MMAP, &mmdb);
    free(path);
    cmp_ok(status, "==", MMDB_SUCCESS, "MMDB_open opens the uint16 key record");
    if (MMDB_SUCCESS == status) {
        int gai_error, mmdb_error;
        MMDB_lookup_result_s result =
            MMDB_lookup_string(&mmdb, "1.1.1.1", &gai_error, &mmdb_error);
        ok(0 == gai_error && MMDB_SUCCESS == mmdb_error && result.found_entry,
           "lookup found the record");

        MMDB_entry_data_list_s *entry_data_list = NULL;
        status = MMDB_get_entry_data_list(&result.entry, &entry_data_list);
        cmp_ok(status,
               "==",
               MMDB_INVALID_DATA_ERROR,
               "MMDB_get_entry_data_list rejects the record");
        ok(NULL == entry_data_list, "the record list is NULL");
        MMDB_free_entry_data_list(entry_data_list);

        MMDB_entry_data_s entry_data;
        status = MMDB_get_value(&result.entry, &entry_data, "ip", NULL);
        cmp_ok(status,
               "==",
               MMDB_INVALID_DATA_ERROR,
               "MMDB_get_value rejects the record");
        MMDB_close(&mmdb);
    }

    // The description metadata is {1: "Test"}.
    path = bad_database_path("libmaxminddb-non-string-description-key.mmdb");
    status = MMDB_open(path, MMDB_MODE_MMAP, &mmdb);
    free(path);
    cmp_ok(status,
           "==",
           MMDB_INVALID_DATA_ERROR,
           "MMDB_open rejects a uint16 key in the description map");
    if (MMDB_SUCCESS == status) {
        MMDB_close(&mmdb);
    }
}

int main(void) {
    plan(NO_PLAN);

    // {"k": "x"}
    CHECK_ENTRY_DATA_LIST("\xE1"
                          "\x41"
                          "k"
                          "\x41"
                          "x",
                          MMDB_SUCCESS,
                          "a string key is accepted");

    // {1: "x"}, where the key is a uint16.
    CHECK_ENTRY_DATA_LIST("\xE1"
                          "\xA1\x01"
                          "\x41"
                          "x",
                          MMDB_INVALID_DATA_ERROR,
                          "a uint16 key is rejected");

    // A map whose key is a pointer to offset 5, followed by the value "x".
    // Offset 5 holds the string "k".
    CHECK_ENTRY_DATA_LIST("\xE1"
                          "\x20\x05"
                          "\x41"
                          "x"
                          "\x41"
                          "k",
                          MMDB_SUCCESS,
                          "a key that points to a string is accepted");

    // As above, but offset 5 holds the uint16 1.
    CHECK_ENTRY_DATA_LIST("\xE1"
                          "\x20\x05"
                          "\x41"
                          "x"
                          "\xA1\x01",
                          MMDB_INVALID_DATA_ERROR,
                          "a key that points to a uint16 is rejected");

    // {1: "x"}, where the key is a one-byte bytes value.
    CHECK_ENTRY_DATA_LIST("\xE1"
                          "\x81\x01"
                          "\x41"
                          "x",
                          MMDB_INVALID_DATA_ERROR,
                          "a bytes key is rejected");

    // {true: "x"}
    CHECK_ENTRY_DATA_LIST("\xE1"
                          "\x01\x07"
                          "\x41"
                          "x",
                          MMDB_INVALID_DATA_ERROR,
                          "a boolean key is rejected");

    // {{}: "x"}
    CHECK_ENTRY_DATA_LIST("\xE1"
                          "\xE0"
                          "\x41"
                          "x",
                          MMDB_INVALID_DATA_ERROR,
                          "a map key is rejected");

    // {[]: "x"}
    CHECK_ENTRY_DATA_LIST("\xE1"
                          "\x00\x04"
                          "\x41"
                          "x",
                          MMDB_INVALID_DATA_ERROR,
                          "an array key is rejected");

    // {"k": {1: "x"}}
    CHECK_ENTRY_DATA_LIST("\xE1"
                          "\x41"
                          "k"
                          "\xE1"
                          "\xA1\x01"
                          "\x41"
                          "x",
                          MMDB_INVALID_DATA_ERROR,
                          "a uint16 key in a nested map is rejected");

    // {"k": [{1: "x"}]}
    CHECK_ENTRY_DATA_LIST("\xE1"
                          "\x41"
                          "k"
                          "\x01\x04"
                          "\xE1"
                          "\xA1\x01"
                          "\x41"
                          "x",
                          MMDB_INVALID_DATA_ERROR,
                          "a uint16 key in a map in an array is rejected");

    check_metadata_as_entry_data_list();

    check_bad_data_files();

    done_testing();
}
