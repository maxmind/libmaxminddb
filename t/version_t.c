#include "maxminddb_test_helper.h"

static void test_version(void **UNUSED(state)) {
    const char *version = MMDB_lib_version();
    assert_non_null(version);
    assert_string_equal(version, PACKAGE_VERSION);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_version),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
