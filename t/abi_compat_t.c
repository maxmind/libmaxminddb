#include "abi_layout.h"
#include "maxminddb_test_helper.h"

#define ABI_SIZE(type) sizeof(type),
#define ABI_FIELD(type, field)                                                 \
    offsetof(type, field), sizeof(((type *)0)->field),
static const size_t current_layout[] = {MMDB_ABI_LAYOUT(ABI_SIZE, ABI_FIELD)};
#undef ABI_SIZE
#undef ABI_FIELD

#define ABI_SIZE(type) "sizeof(" #type ")",
#define ABI_FIELD(type, field)                                                 \
    "offsetof(" #type ", " #field ")", "sizeof(" #type "." #field ")",
static const char *labels[] = {MMDB_ABI_LAYOUT(ABI_SIZE, ABI_FIELD)};
#undef ABI_SIZE
#undef ABI_FIELD

int main(void) {
    plan(NO_PLAN);
    size_t count;
    const size_t *legacy_layout = legacy_abi_layout(&count);
    size_t current_count = sizeof(current_layout) / sizeof(current_layout[0]);
    cmp_ok(count, "==", current_count, "same number of ABI layout checks");
    for (size_t i = 0; i < count && i < current_count; i++) {
        cmp_ok(current_layout[i], "==", legacy_layout[i], "%s", labels[i]);
    }
    char *path = test_database_path("MaxMind-DB-test-mixed-28.mmdb");
    cmp_ok(
        legacy_abi_client(path),
        "==",
        0,
        "old-header client keeps canaries, lookups, moves and cleanup intact");
    free(path);
    done_testing();
}
