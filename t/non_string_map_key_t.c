#include "maxminddb_test_helper.h"
#ifndef _WIN32
    #include <unistd.h>
#endif

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
    MMDB_free_entry_data_list(entry_data_list);
}

#ifndef _WIN32
// A database with one node whose records point to no data, a 4-byte data
// section, and the metadata marker. The metadata follows it.
    #define DATABASE_PREFIX                                                    \
        "\x00\x00\x01\x00\x00\x01"                                             \
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"     \
        "\x43"                                                                 \
        "abc"                                                                  \
        "\xAB\xCD\xEF"                                                         \
        "MaxMind.com"

    #define METADATA(languages, description)                                   \
        "\xE9"                                                                 \
        "\x4A"                                                                 \
        "node_count"                                                           \
        "\xC1\x01"                                                             \
        "\x4B"                                                                 \
        "record_size"                                                          \
        "\xA1\x18"                                                             \
        "\x4A"                                                                 \
        "ip_version"                                                           \
        "\xA1\x04"                                                             \
        "\x4D"                                                                 \
        "database_type"                                                        \
        "\x44"                                                                 \
        "Test"                                                                 \
        "\x5B"                                                                 \
        "binary_format_major_version"                                          \
        "\xA1\x02"                                                             \
        "\x5B"                                                                 \
        "binary_format_minor_version"                                          \
        "\xA0"                                                                 \
        "\x4B"                                                                 \
        "build_epoch"                                                          \
        "\x01\x02\x01"                                                         \
        "\x49"                                                                 \
        "languages" languages "\x4B"                                           \
        "description" description

static void check_open(const char *database,
                       size_t size,
                       int expected_status,
                       const char *message) {
    char filename[64];
    snprintf(filename,
             sizeof(filename),
             "/tmp/non_string_map_key_t.%ld",
             (long)getpid());
    FILE *fp = fopen(filename, "wbx");
    if (NULL == fp) {
        BAIL_OUT("could not create %s", filename);
    }
    size_t written = fwrite(database, 1, size, fp);
    if (0 != fclose(fp) || written != size) {
        BAIL_OUT("could not write the temporary file");
    }

    MMDB_s mmdb;
    int status = MMDB_open(filename, MMDB_MODE_MMAP, &mmdb);
    cmp_ok(status, "==", expected_status, "MMDB_open status: %s", message);
    if (MMDB_SUCCESS == status) {
        MMDB_close(&mmdb);
    }
    unlink(filename);
}

    #define CHECK_OPEN(database, status, message)                              \
        check_open(database, sizeof(database) - 1, status, message)
#endif

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

    check_metadata_as_entry_data_list();

#ifndef _WIN32
    // An empty array is "\x00\x04", and an empty map is "\xE0".
    CHECK_OPEN(DATABASE_PREFIX METADATA("\x00\x04", "\xE0"),
               MMDB_SUCCESS,
               "valid metadata");

    // MMDB_open() reports a non-string key in the description or languages
    // metadata as invalid metadata, not as invalid data.
    CHECK_OPEN(DATABASE_PREFIX METADATA("\x00\x04",
                                        "\xE1"
                                        "\xA1\x01"
                                        "\x41"
                                        "x"),
               MMDB_INVALID_METADATA_ERROR,
               "a uint16 key in description");
    CHECK_OPEN(DATABASE_PREFIX METADATA("\x01\x04"
                                        "\xE1"
                                        "\xA1\x01"
                                        "\x41"
                                        "x",
                                        "\xE0"),
               MMDB_INVALID_METADATA_ERROR,
               "a map with a uint16 key in languages");
#endif

    done_testing();
}
