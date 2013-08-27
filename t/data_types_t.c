#include "MMDB_test_helper.h"

void test_all_data_types(MMDB_lookup_result_s *result, const char *ip,
                         const char *filename, const char *mode_desc)
{
    {
        char description[500];
        snprintf(description, 500, "utf8_string field for %s - %s", ip,
                 mode_desc);

        MMDB_return_s data =
            data_ok(result, MMDB_DTYPE_UTF8_STRING, description, "utf8_string",
                    NULL);
        const char *string = strndup((const char *)data.ptr, data.data_size);
        // This is hex for "unicode! ☯ - ♫" as bytes
        char expect[19] =
            { 0x75, 0x6e, 0x69, 0x63, 0x6f, 0x64, 0x65, 0x21, 0x20, 0xe2, 0x98,
            0xaf, 0x20, 0x2d, 0x20, 0xe2, 0x99, 0xab, 0x00
        };
        is(string, expect, "got expected utf8_string value");

        free((char *)string);
    }

    {
        char description[500];
        snprintf(description, 500, "double field for %s - %s", ip, mode_desc);

        MMDB_return_s data =
            data_ok(result, MMDB_DTYPE_IEEE754_DOUBLE, description, "double",
                    NULL);
        double expect = 42.123456;
        double diff = fabs(data.double_value - expect);
        int is_ok =
            ok(diff < 0.01, "double value was approximately %2.6f", description,
               expect);
        if (!is_ok) {
            diag("  got %2.6f but expected %2.6f (diff = %2.6f)",
                 data.double_value, expect, diff);
        }
    }

    {
        char description[500];
        snprintf(description, 500, "float field for %s - %s", ip, mode_desc);

        MMDB_return_s data =
            data_ok(result, MMDB_DTYPE_IEEE754_FLOAT, description, "float",
                    NULL);
        float expect = 1.1;
        float diff = fabs(data.float_value - expect);
        int is_ok =
            ok(diff < 0.01, "float value was approximately %2.1f", description,
               expect);
        if (!is_ok) {
            diag("  got %2.4f but expected %2.4f (diff = %2.4f)",
                 data.float_value, expect, diff);
        }
    }

    {
        char description[500];
        snprintf(description, 500, "bytes field for %s - %s", ip, mode_desc);

        MMDB_return_s data =
            data_ok(result, MMDB_DTYPE_BYTES, description, "bytes", NULL);
        uint8_t expect[] = { 0x00, 0x00, 0x00, 0x2a };
        ok(memcmp((uint8_t *)data.ptr, expect, 4) == 0,
           "bytes field has expected value");
    }

    {
        char description[500];
        snprintf(description, 500, "uint16 field for %s - %s", ip, mode_desc);

        MMDB_return_s data =
            data_ok(result, MMDB_DTYPE_UINT16, description, "uint16", NULL);
        uint16_t expect = 100;
        ok((uint16_t)data.uinteger == expect, "uint16 field is 100");
    }

    {
        char description[500];
        snprintf(description, 500, "uint32 field for %s - %s", ip, mode_desc);

        MMDB_return_s data =
            data_ok(result, MMDB_DTYPE_UINT32, description, "uint32", NULL);
        uint32_t expect = 1 << 28;
        cmp_ok(expect, "==", data.uinteger, "uint32 field is 2**28");
    }

    {
        char description[500];
        snprintf(description, 500, "uint64 field for %s - %s", ip, mode_desc);

        MMDB_return_s data =
            data_ok(result, MMDB_DTYPE_UINT64, description, "uint64", NULL);
        uint64_t expect = 1;
        expect <<= 60;
        ok(data.uinteger == expect, "uint64 field is 2**60");
    }

    {
        char description[500];
        snprintf(description, 500, "uint128 field for %s - %s", ip, mode_desc);

        MMDB_return_s data =
            data_ok(result, MMDB_DTYPE_UINT128, description, "uint128", NULL);
        uint8_t expect[] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };
        ok(memcmp(data.c16, expect, 16) == 0, "uint128 field is 2**120");
    }

    {
        char description[500];
        snprintf(description, 500, "boolean field for %s - %s", ip, mode_desc);

        MMDB_return_s data =
            data_ok(result, MMDB_DTYPE_BOOLEAN, description, "boolean", NULL);
        cmp_ok(1, "==", data.uinteger, "boolean field is true (1)");
    }

    {
        char description[500];
        snprintf(description, 500, "array field for %s - %s", ip, mode_desc);

        MMDB_return_s data =
            data_ok(result, MMDB_DTYPE_ARRAY, description, "array", NULL);
        cmp_ok(3, "==", data.data_size, "array field has 3 elements");

        snprintf(description, 500, "array[0] for %s - %s", ip, mode_desc);
        data =
            data_ok(result, MMDB_DTYPE_UINT32, description, "array", "0", NULL);
        cmp_ok(1, "==", data.uinteger, "array[0] is 1");

        snprintf(description, 500, "array[1] for %s - %s", ip, mode_desc);
        data =
            data_ok(result, MMDB_DTYPE_UINT32, description, "array", "1", NULL);
        cmp_ok(2, "==", data.uinteger, "array[1] is 1");

        snprintf(description, 500, "array[2] for %s - %s", ip, mode_desc);
        data =
            data_ok(result, MMDB_DTYPE_UINT32, description, "array", "2", NULL);
        cmp_ok(3, "==", data.uinteger, "array[2] is 1");
    }

    {
        char description[500];
        snprintf(description, 500, "map field for %s - %s", ip, mode_desc);

        MMDB_return_s data =
            data_ok(result, MMDB_DTYPE_MAP, description, "map", NULL);
        cmp_ok(1, "==", data.data_size, "map field has 1 element");

        snprintf(description, 500, "map{mapX} for %s - %s", ip, mode_desc);

        data =
            data_ok(result, MMDB_DTYPE_MAP, description, "map", "mapX", NULL);
        cmp_ok(2, "==", data.data_size, "map{mapX} field has 2 elements");

        snprintf(description, 500, "map{mapX}{utf8_stringX} for %s - %s", ip,
                 mode_desc);

        data =
            data_ok(result, MMDB_DTYPE_UTF8_STRING, description, "map", "mapX",
                    "utf8_stringX", NULL);
        const char *string = strndup((const char *)data.ptr, data.data_size);
        is(string, "hello", "map{mapX}{utf8_stringX} is 'hello'");
        free((char *)string);

        snprintf(description, 500, "map{mapX}{arrayX} for %s - %s", ip,
                 mode_desc);
        data =
            data_ok(result, MMDB_DTYPE_ARRAY, description, "map", "mapX",
                    "arrayX", NULL);
        cmp_ok(3, "==", data.data_size,
               "map{mapX}{arrayX} field has 3 elements");

        snprintf(description, 500, "map{mapX}{arrayX}[0] for %s - %s", ip,
                 mode_desc);
        data =
            data_ok(result, MMDB_DTYPE_UINT32, description, "map", "mapX",
                    "arrayX", "0", NULL);
        cmp_ok(7, "==", data.uinteger, "map{mapX}{arrayX}[0] is 7");

        snprintf(description, 500, "map{mapX}{arrayX}[1] for %s - %s", ip,
                 mode_desc);
        data =
            data_ok(result, MMDB_DTYPE_UINT32, description, "map", "mapX",
                    "arrayX", "1", NULL);
        cmp_ok(8, "==", data.uinteger, "map{mapX}{arrayX}[1] is 8");

        snprintf(description, 500, "map{mapX}{arrayX}[2] for %s - %s", ip,
                 mode_desc);
        data =
            data_ok(result, MMDB_DTYPE_UINT32, description, "map", "mapX",
                    "arrayX", "2", NULL);
        cmp_ok(9, "==", data.uinteger, "map{mapX}{arrayX}[2] is 9");
    }

}

void run_tests(int mode, const char *mode_desc)
{
    const char *filename = "MaxMind-DB-test-decoder.mmdb";
    const char *path = test_database_path(filename);
    MMDB_s *mmdb = open_ok(path, mode, mode_desc);

    // All of the remaining tests require an open mmdb
    if (NULL == mmdb) {
        diag("could not open %s - skipping remaining tests", path);
        return;
    }

    {
        const char *ip = "not an ip";

        int gai_error, mmdb_error;
        MMDB_lookup_result_s *result =
            MMDB_lookup(mmdb, ip, &gai_error, &mmdb_error);

        cmp_ok(gai_error, "==", EAI_NONAME,
               "MMDB_lookup populates getaddrinfo error properly - %s", ip);

        ok(NULL == result,
           "no result entry struct returned for invalid IP address '%s'", ip);

        if (NULL != result) {
            free(result);
        }
    }

    {
        const char *ip = "e900::";
        MMDB_lookup_result_s *result = lookup_ok(mmdb, ip, filename, mode_desc);

        ok(NULL == result,
           "no result entry struct returned for IP address not in the database - %s - %s - %s",
           ip, filename, mode_desc);

        if (NULL != result) {
            free(result);
        }
    }

    {
        const char *ip = "::1.1.1.1";
        MMDB_lookup_result_s *result = lookup_ok(mmdb, ip, filename, mode_desc);

        ok(NULL != result,
           "got a result entry struct for IP address in the database - %s - %s - %s",
           ip, filename, mode_desc);

        cmp_ok(result->entry.offset, ">", 0,
               "result.entry.offset > 0 for address in the database - %s - %s - %s",
               ip, filename, mode_desc);

        test_all_data_types(result, ip, filename, mode_desc);

        free(result);
    }

    {
        const char *ip = "::4.5.6.7";
        MMDB_lookup_result_s *result = lookup_ok(mmdb, ip, filename, mode_desc);

        ok(NULL != result,
           "got a result entry struct for IP address in the database - %s - %s - %s",
           ip, filename, mode_desc);

        cmp_ok(result->entry.offset, ">", 0,
               "result.entry.offset > 0 for address in the database - %s - %s - %s",
               ip, filename, mode_desc);

        test_all_data_types(result, ip, filename, mode_desc);

        free(result);
    }

    free((void *)path);
    MMDB_close(mmdb);
}

int main(void)
{
    plan(NO_PLAN);
    for_all_modes(&run_tests);
    done_testing();
}
