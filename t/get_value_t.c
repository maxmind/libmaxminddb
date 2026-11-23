#include "maxminddb_test_helper.h"

void test_array_0_result(int status,
                         MMDB_entry_data_s entry_data,
                         char *function) {
    cmp_ok(status,
           "==",
           MMDB_SUCCESS,
           "status for %s() is MMDB_SUCCESS - array[0]",
           function);
    ok(entry_data.has_data, "found a value for array[0]");
    cmp_ok(entry_data.type,
           "==",
           MMDB_DATA_TYPE_UINT32,
           "returned entry type is uint32 - array[0]");
    cmp_ok(entry_data.uint32, "==", 1, "entry value is 1 - array[0]");
    cmp_ok(entry_data.data_size, "==", 0, "data_size is 0 - array[0]");
}

void test_array_2_result(int status,
                         MMDB_entry_data_s entry_data,
                         char *function) {
    cmp_ok(status,
           "==",
           MMDB_SUCCESS,
           "status for %s() is MMDB_SUCCESS - array[2]",
           function);
    ok(entry_data.has_data, "found a value for array[2]");
    cmp_ok(entry_data.type,
           "==",
           MMDB_DATA_TYPE_UINT32,
           "returned entry type is uint32 - array[2]");
    cmp_ok(entry_data.uint32, "==", 3, "entry value is 3 - array[2]");
}

void test_array_minus_3_result(int status,
                               MMDB_entry_data_s entry_data,
                               char *function) {
    cmp_ok(status,
           "==",
           MMDB_SUCCESS,
           "status for %s() is MMDB_SUCCESS - array[-3]",
           function);
    ok(entry_data.has_data, "found a value for array[-3]");
    cmp_ok(entry_data.type,
           "==",
           MMDB_DATA_TYPE_UINT32,
           "returned entry type is uint32 - array[-3]");
    cmp_ok(entry_data.uint32, "==", 1, "entry value is 1 - array[-3]");
}

void test_array_minus_1_result(int status,
                               MMDB_entry_data_s entry_data,
                               char *function) {
    cmp_ok(status,
           "==",
           MMDB_SUCCESS,
           "status for %s() is MMDB_SUCCESS - array[-1]",
           function);
    ok(entry_data.has_data, "found a value for array[-1]");
    cmp_ok(entry_data.type,
           "==",
           MMDB_DATA_TYPE_UINT32,
           "returned entry type is uint32 - array[-1]");
    cmp_ok(entry_data.uint32, "==", 3, "entry value is 3 - array[-1]");
}

int call_vget_value(MMDB_entry_s *entry, MMDB_entry_data_s *entry_data, ...) {
    va_list keys;
    va_start(keys, entry_data);

    int status = MMDB_vget_value(entry, entry_data, keys);

    va_end(keys);

    return status;
}

void test_simple_structure(int mode, const char *mode_desc) {
    const char *filename = "MaxMind-DB-test-decoder.mmdb";
    char *path = test_database_path(filename);
    MMDB_s *mmdb = open_ok(path, mode, mode_desc);
    free(path);

    const char *ip = "1.1.1.1";
    MMDB_lookup_result_s result =
        lookup_string_ok(mmdb, ip, filename, mode_desc);

    {
        MMDB_entry_data_s entry_data;
        const char *lookup_path[] = {"array", "0", NULL};
        int status = MMDB_aget_value(&result.entry, &entry_data, lookup_path);
        test_array_0_result(status, entry_data, "MMDB_aget_value");

        status = MMDB_get_value(&result.entry, &entry_data, "array", "0", NULL);
        test_array_0_result(status, entry_data, "MMDB_get_value");

        status =
            call_vget_value(&result.entry, &entry_data, "array", "0", NULL);
        test_array_0_result(status, entry_data, "MMDB_vget_value");
    }

    {
        MMDB_entry_data_s entry_data;
        const char *lookup_path[] = {"array", "2", NULL};
        int status = MMDB_aget_value(&result.entry, &entry_data, lookup_path);
        test_array_2_result(status, entry_data, "MMDB_aget_value");

        status = MMDB_get_value(&result.entry, &entry_data, "array", "2", NULL);
        test_array_2_result(status, entry_data, "MMDB_get_value");

        status =
            call_vget_value(&result.entry, &entry_data, "array", "2", NULL);
        test_array_2_result(status, entry_data, "MMDB_vget_value");
    }

    {
        MMDB_entry_data_s entry_data;
        int status =
            MMDB_get_value(&result.entry, &entry_data, "array", "zero", NULL);
        cmp_ok(status,
               "==",
               MMDB_LOOKUP_PATH_DOES_NOT_MATCH_DATA_ERROR,
               "MMDB_get_value() returns error on non-integer array index");
    }

    {
        MMDB_entry_data_s entry_data;
        const char *lookup_path[] = {"array", "-1", NULL};
        int status = MMDB_aget_value(&result.entry, &entry_data, lookup_path);
        test_array_minus_1_result(status, entry_data, "MMDB_aget_value");

        status =
            MMDB_get_value(&result.entry, &entry_data, "array", "-1", NULL);
        test_array_minus_1_result(status, entry_data, "MMDB_get_value");

        status =
            call_vget_value(&result.entry, &entry_data, "array", "-1", NULL);
        test_array_minus_1_result(status, entry_data, "MMDB_vget_value");
    }

    {
        MMDB_entry_data_s entry_data;
        const char *lookup_path[] = {"array", "-3", NULL};
        int status = MMDB_aget_value(&result.entry, &entry_data, lookup_path);
        test_array_minus_3_result(status, entry_data, "MMDB_aget_value");

        status =
            MMDB_get_value(&result.entry, &entry_data, "array", "-3", NULL);
        test_array_minus_3_result(status, entry_data, "MMDB_get_value");

        status =
            call_vget_value(&result.entry, &entry_data, "array", "-3", NULL);
        test_array_minus_3_result(status, entry_data, "MMDB_vget_value");
    }

    {
        MMDB_entry_data_s entry_data;
        int status =
            MMDB_get_value(&result.entry, &entry_data, "array", "-4", NULL);
        cmp_ok(status,
               "==",
               MMDB_LOOKUP_PATH_DOES_NOT_MATCH_DATA_ERROR,
               "MMDB_get_value() returns error on too large negative integer");
    }

    {
        MMDB_entry_data_s entry_data;
        int status = MMDB_get_value(
            &result.entry, &entry_data, "array", "-18446744073709551616", NULL);
        cmp_ok(
            status,
            "==",
            MMDB_INVALID_LOOKUP_PATH_ERROR,
            "MMDB_get_value() returns error on integer smaller than LONG_MIN");
    }

    {
        MMDB_entry_data_s entry_data;
        int status = MMDB_get_value(
            &result.entry, &entry_data, "array", "18446744073709551616", NULL);
        cmp_ok(
            status,
            "==",
            MMDB_INVALID_LOOKUP_PATH_ERROR,
            "MMDB_get_value() returns error on integer larger than LONG_MAX");
    }

    MMDB_close(mmdb);
    free(mmdb);
}

void test_complex_map_a_result(int status,
                               MMDB_entry_data_s entry_data,
                               char *function) {
    cmp_ok(status,
           "==",
           MMDB_SUCCESS,
           "status for %s() is MMDB_SUCCESS - map1{map2}{array}[0]{map3}{a}",
           function);
    ok(entry_data.has_data, "found a value for map1{map2}{array}[0]{map3}{a}");
    cmp_ok(entry_data.type,
           "==",
           MMDB_DATA_TYPE_UINT32,
           "returned entry type is uint32 - map1{map2}{array}[0]{map3}{a}");
    cmp_ok(entry_data.uint32,
           "==",
           1,
           "entry value is 1 - map1{map2}{array}[0]{map3}{a}");
    cmp_ok(entry_data.data_size,
           "==",
           0,
           "data_size is 0 - map1{map2}{array}[0]{map3}{a}");
}

void test_complex_map_c_result(int status,
                               MMDB_entry_data_s entry_data,
                               char *function) {
    cmp_ok(status,
           "==",
           MMDB_SUCCESS,
           "status for %s() is MMDB_SUCCESS - map1{map2}{array}[0]{map3}{c}",
           function);
    ok(entry_data.has_data, "found a value for map1{map2}{array}[0]{map3}{c}");
    cmp_ok(entry_data.type,
           "==",
           MMDB_DATA_TYPE_UINT32,
           "returned entry type is uint32 - map1{map2}{array}[0]{map3}{c}");
    cmp_ok(entry_data.uint32,
           "==",
           3,
           "entry value is 3 - map1{map2}{array}[0]{map3}{c}");
}

void test_no_result(int status,
                    MMDB_entry_data_s entry_data,
                    char *function,
                    char *path_description) {
    cmp_ok(status,
           "==",
           MMDB_LOOKUP_PATH_DOES_NOT_MATCH_DATA_ERROR,
           "status for %s() is MMDB_LOOKUP_PATH_DOES_NOT_MATCH_DATA_ERROR - %s",
           function,
           path_description);
    ok(!entry_data.has_data, "did not find a value for %s", path_description);
}

void test_nested_structure(int mode, const char *mode_desc) {
    const char *filename = "MaxMind-DB-test-nested.mmdb";
    char *path = test_database_path(filename);
    MMDB_s *mmdb = open_ok(path, mode, mode_desc);
    free(path);

    const char *ip = "1.1.1.1";
    MMDB_lookup_result_s result =
        lookup_string_ok(mmdb, ip, filename, mode_desc);

    {
        MMDB_entry_data_s entry_data;
        const char *lookup_path[] = {
            "map1", "map2", "array", "0", "map3", "a", NULL};
        int status = MMDB_aget_value(&result.entry, &entry_data, lookup_path);
        test_complex_map_a_result(status, entry_data, "MMDB_aget_value");

        status = MMDB_get_value(&result.entry,
                                &entry_data,
                                "map1",
                                "map2",
                                "array",
                                "0",
                                "map3",
                                "a",
                                NULL);
        test_complex_map_a_result(status, entry_data, "MMDB_get_value");

        status = call_vget_value(&result.entry,
                                 &entry_data,
                                 "map1",
                                 "map2",
                                 "array",
                                 "0",
                                 "map3",
                                 "a",
                                 NULL);
        test_complex_map_a_result(status, entry_data, "MMDB_vget_value");
    }

    {
        MMDB_entry_data_s entry_data;
        const char *lookup_path[] = {
            "map1", "map2", "array", "0", "map3", "c", NULL};
        int status = MMDB_aget_value(&result.entry, &entry_data, lookup_path);
        test_complex_map_c_result(status, entry_data, "MMDB_aget_value");

        status = MMDB_get_value(&result.entry,
                                &entry_data,
                                "map1",
                                "map2",
                                "array",
                                "0",
                                "map3",
                                "c",
                                NULL);
        test_complex_map_c_result(status, entry_data, "MMDB_get_value");

        status = call_vget_value(&result.entry,
                                 &entry_data,
                                 "map1",
                                 "map2",
                                 "array",
                                 "0",
                                 "map3",
                                 "c",
                                 NULL);
        test_complex_map_c_result(status, entry_data, "MMDB_vget_value");
    }

    {
        MMDB_entry_data_s entry_data;
        const char *lookup_path[] = {
            "map1", "map42", "array", "0", "map3", "c", NULL};
        int status = MMDB_aget_value(&result.entry, &entry_data, lookup_path);
        test_no_result(status,
                       entry_data,
                       "MMDB_aget_value",
                       "map1{map42}{array}[0]{map3}{c}");

        status = MMDB_get_value(&result.entry,
                                &entry_data,
                                "map1",
                                "map42",
                                "array",
                                "0",
                                "map3",
                                "c",
                                NULL);
        test_no_result(status,
                       entry_data,
                       "MMDB_get_value",
                       "map1{map42}{array}[0]{map3}{c}");

        status = call_vget_value(&result.entry,
                                 &entry_data,
                                 "map1",
                                 "map42",
                                 "array",
                                 "0",
                                 "map3",
                                 "c",
                                 NULL);
        test_no_result(status,
                       entry_data,
                       "MMDB_vget_value",
                       "map1{map42}{array}[0]{map3}{c}");
    }

    {
        MMDB_entry_data_s entry_data;
        const char *lookup_path[] = {
            "map1", "map2", "array", "9", "map3", "c", NULL};
        int status = MMDB_aget_value(&result.entry, &entry_data, lookup_path);
        test_no_result(status,
                       entry_data,
                       "MMDB_aget_value",
                       "map1{map42}{array}[9]{map3}{c}");

        status = MMDB_get_value(&result.entry,
                                &entry_data,
                                "map1",
                                "map2",
                                "array",
                                "9",
                                "map3",
                                "c",
                                NULL);
        test_no_result(status,
                       entry_data,
                       "MMDB_get_value",
                       "map1{map42}{array}[9]{map3}{c}");

        status = call_vget_value(&result.entry,
                                 &entry_data,
                                 "map1",
                                 "map2",
                                 "array",
                                 "9",
                                 "map3",
                                 "c",
                                 NULL);
        test_no_result(status,
                       entry_data,
                       "MMDB_vget_value",
                       "map1{map42}{array}[9]{map3}{c}");
    }

    MMDB_close(mmdb);
    free(mmdb);
}

void run_tests(int mode, const char *mode_desc) {
    test_simple_structure(mode, mode_desc);
    test_nested_structure(mode, mode_desc);
}

static void check_pointer_target(MMDB_s *mmdb,
                                 uint32_t offset,
                                 const char *const *path,
                                 uint32_t expected_type,
                                 const char *message) {
    MMDB_entry_s entry = {.mmdb = mmdb, .offset = offset};
    MMDB_entry_data_s entry_data;
    // The call must replace all of these bytes.
    memset(&entry_data, 0xFF, sizeof(MMDB_entry_data_s));

    int status = MMDB_aget_value(&entry, &entry_data, path);
    cmp_ok(status, "==", MMDB_SUCCESS, "status: %s", message);
    cmp_ok(entry_data.type, "==", expected_type, "type: %s", message);
    cmp_ok(entry_data.data_size, "==", 0, "data_size is 0: %s", message);
    // uint32 shares its bytes with the pointer that the decoder followed.
    cmp_ok(entry_data.uint32, "==", 0, "unused bytes are 0: %s", message);
}

// A pointer sets data_size and four bytes of the value. The target of the
// pointer must not keep them.
void test_pointer_to_number_or_boolean(void) {
    // A map with the keys "a" and "b". Each value is a pointer.
    static const char map[] = "\xE2"
                              "\x41"
                              "a"
                              "\x38\x00\x00\x01\x00"
                              "\x41"
                              "b"
                              "\x38\x00\x00\x01\x02";
    uint8_t data[0x0103] = {0};
    memcpy(data, map, sizeof(map) - 1);
    // Offset 0x0100 holds a false boolean, which sets one byte of the value.
    data[0x0101] = 0x07;
    // Offset 0x0102 holds a uint16 of 0, which does not set data_size.
    data[0x0102] = 0xA0;
    MMDB_s mmdb = {.data_section = data, .data_section_size = sizeof(data)};

    const char *const path_a[] = {"a", NULL};
    const char *const path_b[] = {"b", NULL};
    const char *const no_path[] = {NULL};
    check_pointer_target(
        &mmdb, 0, path_a, MMDB_DATA_TYPE_BOOLEAN, "boolean in a map");
    check_pointer_target(
        &mmdb, 0, path_b, MMDB_DATA_TYPE_UINT16, "uint16 in a map");
    check_pointer_target(
        &mmdb, 3, no_path, MMDB_DATA_TYPE_BOOLEAN, "boolean with no path");
    check_pointer_target(
        &mmdb, 10, no_path, MMDB_DATA_TYPE_UINT16, "uint16 with no path");
}

int main(void) {
    plan(NO_PLAN);
    for_all_modes(&run_tests);
    test_pointer_to_number_or_boolean();
    done_testing();
}
