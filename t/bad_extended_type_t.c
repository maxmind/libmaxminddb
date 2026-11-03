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
    if (MMDB_SUCCESS == status) {
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
    // The control byte 0x00 is an extended type with a size of 0.
    CHECK_ENTRY_DATA_LIST("\x00\x00",
                          MMDB_INVALID_DATA_ERROR,
                          0,
                          "extended type byte 0 is rejected");
    CHECK_ENTRY_DATA_LIST("\x00\x09",
                          MMDB_INVALID_DATA_ERROR,
                          0,
                          "extended type byte 9 is rejected");
    CHECK_ENTRY_DATA_LIST("\x00\xFF",
                          MMDB_INVALID_DATA_ERROR,
                          0,
                          "extended type byte 255 is rejected");

    CHECK_ENTRY_DATA_LIST("\x00\x01",
                          MMDB_SUCCESS,
                          MMDB_DATA_TYPE_INT32,
                          "extended type byte 1 is an int32");
    CHECK_ENTRY_DATA_LIST("\x04\x08\x00\x00\x00\x00",
                          MMDB_SUCCESS,
                          MMDB_DATA_TYPE_FLOAT,
                          "extended type byte 8 is a float");
}

static void
check_skipped_value(const char *section, size_t size, const char *message) {
    uint8_t *data = make_section(section, size);
    MMDB_s mmdb = {.data_section = data, .data_section_size = (uint32_t)size};
    MMDB_entry_s entry = {.mmdb = &mmdb, .offset = 0};
    MMDB_entry_data_s entry_data;

    int status = MMDB_get_value(&entry, &entry_data, "b", NULL);
    cmp_ok(status, "==", MMDB_INVALID_DATA_ERROR, "%s", message);

    free(data);
}

void test_extended_type_in_skipped_value(void) {
    // A map of {"a": <bad extended type byte>, "b": 5}. MMDB_get_value skips
    // the value of "a" on its way to "b". Byte 0 is an empty map if it is
    // not rejected, so a skip would silently succeed.
    const char section_9[] = "\xE2"
                             "\x41"
                             "a"
                             "\x00\x09"
                             "\x41"
                             "b"
                             "\xA1\x05";
    check_skipped_value(
        section_9,
        sizeof(section_9) - 1,
        "a skipped value with extended type byte 9 is rejected");

    const char section_0[] = "\xE2"
                             "\x41"
                             "a"
                             "\x00\x00"
                             "\x41"
                             "b"
                             "\xA1\x05";
    check_skipped_value(
        section_0,
        sizeof(section_0) - 1,
        "a skipped value with extended type byte 0 is rejected");
}

int main(void) {
    plan(NO_PLAN);
    test_extended_type_range();
    test_extended_type_in_skipped_value();
    done_testing();
}
