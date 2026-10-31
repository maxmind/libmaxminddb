#include "abi_layout.h"
#include "abi_legacy.h"
#include <string.h>

#define ABI_SIZE(type) sizeof(type),
#define ABI_FIELD(type, field)                                                 \
    offsetof(type, field), sizeof(((type *)0)->field),
const size_t *legacy_abi_layout(size_t *count) {
    static const size_t layout[] = {MMDB_ABI_LAYOUT(ABI_SIZE, ABI_FIELD)};
    *count = sizeof(layout) / sizeof(layout[0]);
    return layout;
}
#undef ABI_SIZE
#undef ABI_FIELD

typedef struct {
    uint64_t before[4];
    MMDB_s reader;
    uint64_t after[4];
} guarded_reader_s;

static void init_guards(guarded_reader_s *guarded) {
    memset(guarded, 0xa5, sizeof(*guarded));
    for (size_t i = 0; i < 4; i++) {
        guarded->before[i] = UINT64_C(0x3a1d635f48c902be) + i;
        guarded->after[i] = UINT64_C(0xb71304e25698f0ca) + i;
    }
}

static bool guards_intact(const guarded_reader_s *guarded) {
    for (size_t i = 0; i < 4; i++) {
        if (guarded->before[i] != UINT64_C(0x3a1d635f48c902be) + i ||
            guarded->after[i] != UINT64_C(0xb71304e25698f0ca) + i) {
            return false;
        }
    }
    return true;
}

static bool lookup_ok(MMDB_s *reader, const char *path) {
    if (strcmp(reader->filename, path)) {
        return false;
    }
    const char *addresses[] = {"1.1.1.1", "::2:0:1a"};
    const char *values[] = {"::1.1.1.1", "::2:0:0"};
    for (size_t i = 0; i < 2; i++) {
        int gai_error, mmdb_error;
        MMDB_lookup_result_s result =
            MMDB_lookup_string(reader, addresses[i], &gai_error, &mmdb_error);
        if (gai_error || mmdb_error || !result.found_entry ||
            result.entry.mmdb != reader) {
            return false;
        }
        MMDB_entry_data_s data;
        if (MMDB_get_value(&result.entry, &data, "ip", NULL) ||
            !data.has_data || data.type != 2 ||
            data.data_size != strlen(values[i]) ||
            memcmp(data.utf8_string, values[i], data.data_size)) {
            return false;
        }
        if (i == 0) {
            struct sockaddr_in address = {.sin_family = AF_INET};
            memset(&address.sin_addr, 1, sizeof(address.sin_addr));
            MMDB_lookup_result_s socket_result = MMDB_lookup_sockaddr(
                reader, (const struct sockaddr *)&address, &mmdb_error);
            if (mmdb_error || !socket_result.found_entry ||
                socket_result.entry.mmdb != reader ||
                socket_result.entry.offset != result.entry.offset ||
                socket_result.netmask != result.netmask) {
                return false;
            }
        }
    }
    return true;
}

static MMDB_s open_by_value(const char *path, int *status) {
    MMDB_s reader;
    *status = MMDB_open(path, 1, &reader);
    return reader;
}

int legacy_abi_client(const char *path) {
    guarded_reader_s guarded;
    init_guards(&guarded);
    if (MMDB_open(path, 1, &guarded.reader) || !guards_intact(&guarded)) {
        return 1;
    }
    bool valid = lookup_ok(&guarded.reader, path);
    uint32_t flags = guarded.reader.flags;
    MMDB_close(&guarded.reader);
    MMDB_close(&guarded.reader);
    if (!valid || !guards_intact(&guarded)) {
        return 2;
    }
    if (MMDB_open(path, flags, &guarded.reader)) {
        return 3;
    }
    valid = lookup_ok(&guarded.reader, path);
    MMDB_close(&guarded.reader);
    if (!valid || !guards_intact(&guarded)) {
        return 4;
    }
    int status;
    MMDB_s returned = open_by_value(path, &status);
    if (status) {
        return 5;
    }
    memcpy(&guarded.reader, &returned, sizeof(returned));
    volatile unsigned char *bytes = (volatile unsigned char *)&returned;
    for (size_t i = 0; i < sizeof(returned); i++) {
        bytes[i] = 0;
    }
    valid = lookup_ok(&guarded.reader, path);
    MMDB_close(&guarded.reader);
    return valid && guards_intact(&guarded) ? 0 : 6;
}
