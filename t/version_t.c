#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include "maxminddb_test_helper.h"

int main(void) {
    const char *version = MMDB_lib_version();
    if (ok((version != NULL), "MMDB_lib_version exists")) {
        is(version, PACKAGE_VERSION, "version is " PACKAGE_VERSION);
    }
    done_testing();
}
