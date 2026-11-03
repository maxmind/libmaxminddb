#include "maxminddb_test_helper.h"
#ifndef _WIN32
    #include <pthread.h>
#endif

typedef union {
    struct sockaddr sa;
    struct sockaddr_in v4;
    struct sockaddr_in6 v6;
} address_s;

static uint32_t next_random(uint32_t *state) {
    *state ^= *state << 13;
    *state ^= *state >> 17;
    *state ^= *state << 5;
    return *state;
}

static address_s make_query(unsigned int i, uint32_t *state) {
    address_s address;
    memset(&address, 0, sizeof(address));
    if (i % 17 == 0) {
        address.sa.sa_family = AF_UNSPEC;
        return address;
    }
    uint8_t *bytes;
    unsigned int size;
    if (i % 2) {
        address.v4.sin_family = AF_INET;
        bytes = (uint8_t *)&address.v4.sin_addr;
        size = 4;
    } else {
        address.v6.sin6_family = AF_INET6;
        bytes = (uint8_t *)&address.v6.sin6_addr;
        size = 16;
    }
    for (unsigned int j = 0; j < size; j++) {
        bytes[j] = (uint8_t)next_random(state);
    }
    if (i % 3 == 0) {
        memset(bytes, 0, size);
        if (size == 4) {
            bytes[0] = bytes[1] = bytes[2] = 1;
            bytes[3] = (uint8_t)i;
        } else {
            bytes[11] = 2;
            bytes[15] = (uint8_t)i;
        }
    } else if (i % 5 == 0) {
        memset(bytes, i & 2 ? 0 : 255, size);
    }
    return address;
}

static bool same_result(MMDB_lookup_result_s expected,
                        int expected_error,
                        MMDB_lookup_result_s actual,
                        int actual_error) {
    return expected_error == actual_error &&
           expected.found_entry == actual.found_entry &&
           expected.netmask == actual.netmask &&
           expected.entry.mmdb == actual.entry.mmdb &&
           expected.entry.offset == actual.entry.offset;
}

/* Keep the oracle on the packed traversal even when open prepared a reader. */
static MMDB_lookup_result_s packed_sockaddr(const MMDB_s *mmdb,
                                            const struct sockaddr *address,
                                            int *error) {
    MMDB_s packed = *mmdb;
    packed.flags |= MMDB_NO_PREPARE;
    MMDB_lookup_result_s result = MMDB_lookup_sockaddr(&packed, address, error);
    result.entry.mmdb = mmdb;
    return result;
}

static bool same_sockaddr(const MMDB_s *mmdb,
                          const MMDB_lookup_context_s *context,
                          const struct sockaddr *address) {
    int expected_error, actual_error, automatic_error;
    MMDB_lookup_result_s expected =
        packed_sockaddr(mmdb, address, &expected_error);
    MMDB_lookup_result_s actual =
        MMDB_lookup_sockaddr_with_context(context, address, &actual_error);
    MMDB_lookup_result_s automatic =
        MMDB_lookup_sockaddr(mmdb, address, &automatic_error);
    return same_result(expected, expected_error, actual, actual_error) &&
           same_result(expected, expected_error, automatic, automatic_error);
}

static MMDB_lookup_context_s *context_ok(const MMDB_s *mmdb,
                                         const char *label) {
    MMDB_lookup_context_s *context = NULL;
    int error = MMDB_create_lookup_context(mmdb, &context);
    cmp_ok(error, "==", MMDB_SUCCESS, "create lookup context - %s", label);
    ok(context != NULL, "lookup context is returned - %s", label);
    return context;
}

static void check_strings(const MMDB_s *mmdb,
                          const MMDB_lookup_context_s *context,
                          const char *label) {
    const char *addresses[] = {"1.1.1.1",
                               "1.1.1.3",
                               "0.0.0.0",
                               "255.255.255.255",
                               "::",
                               "::2:0:1",
                               "::ffff:1.1.1.1",
                               "2001:db8::1",
                               "not-an-address",
                               "999.999.999.999",
                               "1.1.1.1/24",
                               ""};
    unsigned int mismatches = 0;
    MMDB_s packed = *mmdb;
    packed.flags |= MMDB_NO_PREPARE;
    for (size_t i = 0; i < sizeof(addresses) / sizeof(addresses[0]); i++) {
        int expected_gai = -1, expected_error = -1;
        int actual_gai = -1, actual_error = -1;
        int automatic_gai = -1, automatic_error = -1;
        MMDB_lookup_result_s expected = MMDB_lookup_string(
            &packed, addresses[i], &expected_gai, &expected_error);
        expected.entry.mmdb = mmdb;
        MMDB_lookup_result_s actual = MMDB_lookup_string_with_context(
            context, addresses[i], &actual_gai, &actual_error);
        MMDB_lookup_result_s automatic = MMDB_lookup_string(
            mmdb, addresses[i], &automatic_gai, &automatic_error);
        if (expected_gai != actual_gai || expected_gai != automatic_gai ||
            !same_result(expected, expected_error, actual, actual_error) ||
            !same_result(
                expected, expected_error, automatic, automatic_error)) {
            mismatches++;
        }
    }
    cmp_ok(mismatches,
           "==",
           0,
           "string lookups and numeric-address errors match - %s",
           label);
}

#ifndef _WIN32
typedef struct {
    const MMDB_s *mmdb;
    const MMDB_lookup_context_s *context;
    uint32_t seed;
    unsigned int mismatches;
} worker_s;

static void *shared_context_worker(void *argument) {
    worker_s *worker = argument;
    uint32_t state = worker->seed;
    for (unsigned int i = 0; i < 4096; i++) {
        address_s address = make_query(i, &state);
        if (!same_sockaddr(worker->mmdb, worker->context, &address.sa)) {
            worker->mismatches++;
        }
    }
    return NULL;
}

static void check_shared_context(const MMDB_s *mmdb,
                                 const MMDB_lookup_context_s *context,
                                 const char *label) {
    pthread_t threads[4];
    worker_s workers[4];
    for (unsigned int i = 0; i < 4; i++) {
        workers[i] = (worker_s){
            .mmdb = mmdb, .context = context, .seed = 0x187655a1U + i};
        if (pthread_create(
                &threads[i], NULL, shared_context_worker, &workers[i])) {
            BAIL_OUT("could not create lookup-context worker");
        }
    }
    unsigned int mismatches = 0;
    for (unsigned int i = 0; i < 4; i++) {
        if (pthread_join(threads[i], NULL)) {
            BAIL_OUT("could not join lookup-context worker");
        }
        mismatches += workers[i].mismatches;
    }
    cmp_ok(mismatches,
           "==",
           0,
           "four workers share the immutable lookup context - %s",
           label);
}
#endif

static void run_database(int UNUSED(record_size),
                         const char *filename,
                         const char *UNUSED(description)) {
    char *path = test_database_path(filename);
    MMDB_s *mmdb = open_ok(path, MMDB_MODE_MMAP, "mmap mode");
    MMDB_s *packed = open_ok(
        path, MMDB_MODE_MMAP | MMDB_NO_PREPARE, "mmap without preparation");
    free(path);
    if (!mmdb || !packed) {
        MMDB_close(mmdb);
        MMDB_close(packed);
        free(mmdb);
        free(packed);
        return;
    }
    ok((packed->flags & MMDB_NO_PREPARE) != 0,
       "open preserves the packed traversal option - %s",
       filename);
    MMDB_lookup_context_s *context = context_ok(mmdb, filename);
    if (!context) {
        MMDB_close(mmdb);
        MMDB_close(packed);
        free(mmdb);
        free(packed);
        return;
    }
    uint32_t state = 0x185ebca1;
    unsigned int mismatches = 0;
    MMDB_entry_s saved_entry = {.mmdb = NULL};
    for (unsigned int i = 0; i < 4096; i++) {
        address_s address = make_query(i, &state);
        int expected_error, actual_error, automatic_error, opt_out_error;
        MMDB_lookup_result_s expected =
            packed_sockaddr(mmdb, &address.sa, &expected_error);
        MMDB_lookup_result_s actual = MMDB_lookup_sockaddr_with_context(
            context, &address.sa, &actual_error);
        MMDB_lookup_result_s automatic =
            MMDB_lookup_sockaddr(mmdb, &address.sa, &automatic_error);
        MMDB_lookup_result_s opt_out =
            MMDB_lookup_sockaddr(packed, &address.sa, &opt_out_error);
        if (opt_out.entry.mmdb != packed) {
            mismatches++;
        }
        opt_out.entry.mmdb = mmdb;
        if (!same_result(expected, expected_error, actual, actual_error) ||
            !same_result(
                expected, expected_error, automatic, automatic_error) ||
            !same_result(expected, expected_error, opt_out, opt_out_error)) {
            mismatches++;
        }
        if (!actual_error && actual.found_entry && !saved_entry.mmdb) {
            saved_entry = actual.entry;
        }
    }
    cmp_ok(
        mismatches,
        "==",
        0,
        "automatic, explicit and opt-out results match packed traversal - %s",
        filename);
    ok(saved_entry.mmdb != NULL,
       "fixture queries include successful lookups - %s",
       filename);
    check_strings(mmdb, context, filename);
    struct sockaddr invalid = {.sa_family = AF_UNSPEC};
    ok(same_sockaddr(mmdb, context, &invalid),
       "minimal unsupported sockaddr is rejected - %s",
       filename);
#ifndef _WIN32
    check_shared_context(mmdb, context, filename);
#endif
    MMDB_entry_data_s before = {0};
    int before_error = MMDB_SUCCESS;
    if (saved_entry.mmdb) {
        before_error = MMDB_get_value(&saved_entry, &before, "ip", NULL);
    }
    MMDB_free_lookup_context(context);
    if (saved_entry.mmdb) {
        MMDB_entry_data_s after = {0};
        int after_error = MMDB_get_value(&saved_entry, &after, "ip", NULL);
        ok(before_error == MMDB_SUCCESS && after_error == MMDB_SUCCESS &&
               before.has_data && before.type == MMDB_DATA_TYPE_UTF8_STRING &&
               after.has_data && after.type == before.type &&
               after.data_size == before.data_size &&
               !memcmp(after.utf8_string, before.utf8_string, after.data_size),
           "entry decodes through the base reader after context destruction - "
           "%s",
           filename);
    }
    MMDB_close(mmdb);
    MMDB_close(packed);
    free(mmdb);
    free(packed);
}

static void
write_node(uint8_t *p, unsigned int width, uint32_t left, uint32_t right) {
    if (width == 6) {
        p[0] = (uint8_t)(left >> 16);
        p[1] = (uint8_t)(left >> 8);
        p[2] = (uint8_t)left;
        p[3] = (uint8_t)(right >> 16);
        p[4] = (uint8_t)(right >> 8);
        p[5] = (uint8_t)right;
    } else if (width == 7) {
        p[0] = (uint8_t)(left >> 16);
        p[1] = (uint8_t)(left >> 8);
        p[2] = (uint8_t)left;
        p[3] = (uint8_t)(((left >> 24) << 4) | ((right >> 24) & 15));
        p[4] = (uint8_t)(right >> 16);
        p[5] = (uint8_t)(right >> 8);
        p[6] = (uint8_t)right;
    } else {
        for (unsigned int j = 0; j < 4; j++) {
            p[j] = (uint8_t)(left >> (24 - 8 * j));
            p[j + 4] = (uint8_t)(right >> (24 - 8 * j));
        }
    }
}

static MMDB_s
borrowed_reader(uint8_t *raw, unsigned int count, unsigned int width) {
    MMDB_s mmdb = {.flags = MMDB_MODE_MMAP,
                   .file_content = raw,
                   .file_size = (ssize_t)(count * width + 18),
                   .data_section = raw + count * width + 16,
                   .data_section_size = 2,
                   .full_record_byte_size = (uint16_t)width,
                   .depth = 128,
                   .metadata = {.node_count = count,
                                .record_size = (uint16_t)(width * 4),
                                .ip_version = 6}};
    return mmdb;
}

static void check_prefixes(void) {
    const unsigned int depths[] = {
        1, 7, 8, 15, 16, 17, 20, 32, 64, 96, 127, 128};
    for (unsigned int width = 6; width <= 8; width++) {
        for (size_t d = 0; d < sizeof(depths) / sizeof(depths[0]); d++) {
            unsigned int count = depths[d];
            uint8_t raw[128 * 8 + 18] = {0};
            for (unsigned int i = 0; i < count; i++) {
                uint32_t next = i + 1 == count ? count + 16 : i + 1;
                write_node(raw + i * width,
                           width,
                           i % 2 ? count : next,
                           i % 2 ? next : count);
            }
            raw[count * width + 16] = 0xc1;
            raw[count * width + 17] = 42;
            MMDB_s mmdb = borrowed_reader(raw, count, width);
            MMDB_lookup_context_s *context =
                context_ok(&mmdb, "crafted prefix");
            if (!context)
                continue;
            struct sockaddr_in6 address = {.sin6_family = AF_INET6};
            memset(&address.sin6_addr, 0x55, 16);
            int error;
            MMDB_lookup_result_s result = MMDB_lookup_sockaddr_with_context(
                context, (const struct sockaddr *)&address, &error);
            ok(!error && result.found_entry && result.netmask == count &&
                   result.entry.offset == 0 && result.entry.mmdb == &mmdb,
               "%u-bit records preserve exact /%u match depth",
               width * 4,
               count);
            unsigned int mismatches = 0;
            for (unsigned int bit = 0; bit < count; bit++) {
                uint8_t *bytes = (uint8_t *)&address.sin6_addr;
                bytes[bit / 8] ^= (uint8_t)(1U << (7 - bit % 8));
                if (!same_sockaddr(
                        &mmdb, context, (const struct sockaddr *)&address))
                    mismatches++;
                bytes[bit / 8] ^= (uint8_t)(1U << (7 - bit % 8));
            }
            cmp_ok(mismatches,
                   "==",
                   0,
                   "%u-bit records preserve misses at every depth through /%u",
                   width * 4,
                   count);
            MMDB_free_lookup_context(context);
        }
    }
}

static void check_cached_starts(void) {
    uint8_t raw[32 * 7 + 18] = {0};
    for (unsigned int i = 0; i < 32; i++) {
        uint32_t next = i == 31 ? 48 : i + 1;
        write_node(raw + i * 7, 7, i % 2 ? 32 : next, i % 2 ? next : 32);
    }
    raw[32 * 7 + 16] = 0xc1;
    raw[32 * 7 + 17] = 42;
    MMDB_s mmdb = borrowed_reader(raw, 32, 7);
    struct sockaddr_in address = {.sin_family = AF_INET};
    memset(&address.sin_addr, 0xaa, 4);
    const uint16_t starts[] = {95, 111, 112, 113, 127, 128};
    unsigned int mismatches = 0;
    for (size_t i = 0; i < sizeof(starts) / sizeof(starts[0]); i++) {
        mmdb.ipv4_start_node =
            (MMDB_ipv4_start_node_s){.node_value = 2, .netmask = starts[i]};
        MMDB_lookup_context_s *context = context_ok(&mmdb, "cached IPv4 start");
        if (!context)
            continue;
        if (!same_sockaddr(&mmdb, context, (const struct sockaddr *)&address))
            mismatches++;
        if (starts[i] == 95) {
            int error;
            MMDB_lookup_result_s result = MMDB_lookup_sockaddr_with_context(
                context, (const struct sockaddr *)&address, &error);
            ok(!error && result.found_entry && result.netmask == 125 &&
                   result.entry.offset == 0,
               "unaligned cached start preserves the exact successful prefix");
        }
        MMDB_free_lookup_context(context);
    }
    cmp_ok(mismatches,
           "==",
           0,
           "cached IPv4 starts preserve unaligned and exhausted depths");
    const uint32_t terminals[] = {32, 48};
    for (size_t i = 0; i < sizeof(terminals) / sizeof(terminals[0]); i++) {
        mmdb.ipv4_start_node =
            (MMDB_ipv4_start_node_s){.node_value = terminals[i], .netmask = 95};
        MMDB_lookup_context_s *context =
            context_ok(&mmdb, "terminal IPv4 start");
        if (context) {
            ok(same_sockaddr(&mmdb, context, (const struct sockaddr *)&address),
               "cached terminal %u performs no child read",
               terminals[i]);
            MMDB_free_lookup_context(context);
        }
    }
}

static void check_corrupt_trees(void) {
    for (unsigned int width = 6; width <= 8; width++) {
        uint8_t raw[128 * 8 + 18] = {0};
        MMDB_s mmdb = borrowed_reader(raw, 1, width);
        write_node(
            raw, width, 17, 2); /* Left is valid data; right is reserved. */
        raw[width + 16] = 0xc1;
        raw[width + 17] = 42;
        MMDB_lookup_context_s *context =
            context_ok(&mmdb, "unvisited corrupt sibling");
        struct sockaddr_in6 address = {.sin6_family = AF_INET6};
        if (context) {
            ok(same_sockaddr(&mmdb, context, (const struct sockaddr *)&address),
               "corrupt sibling does not reject a valid lookup - %u bits",
               width * 4);
            ((uint8_t *)&address.sin6_addr)[0] = 0x80;
            ok(same_sockaddr(&mmdb, context, (const struct sockaddr *)&address),
               "visited reserved record has the ordinary error - %u bits",
               width * 4);
            MMDB_free_lookup_context(context);
        }
        mmdb = borrowed_reader(raw, 128, width);
        for (unsigned int i = 0; i < 128; i++) {
            write_node(raw + i * width, width, (i + 1) % 128, (i + 7) % 128);
        }
        context = context_ok(&mmdb, "cyclic search tree");
        if (context) {
            uint32_t state = 0x9348abcd;
            unsigned int mismatches = 0;
            for (unsigned int i = 0; i < 256; i++) {
                address_s query = make_query(i, &state);
                if (!same_sockaddr(&mmdb, context, &query.sa))
                    mismatches++;
            }
            cmp_ok(mismatches,
                   "==",
                   0,
                   "cycles stop at the existing depth limit - %u bits",
                   width * 4);
            MMDB_free_lookup_context(context);
        }
        mmdb.data_section = raw + width - 1;
        context = context_ok(&mmdb, "truncated source view");
        if (context) {
            ok(same_sockaddr(&mmdb, context, (const struct sockaddr *)&address),
               "unreadable node fails at lookup before child access - %u bits",
               width * 4);
            MMDB_free_lookup_context(context);
        }
    }
}

static void check_creation_errors(void) {
    uint8_t raw[32] = {0};
    MMDB_s mmdb = borrowed_reader(raw, 1, 3);
    MMDB_lookup_context_s *context = (MMDB_lookup_context_s *)(uintptr_t)1;
    int error = MMDB_create_lookup_context(&mmdb, &context);
    cmp_ok(error,
           "==",
           MMDB_UNKNOWN_DATABASE_FORMAT_ERROR,
           "unsupported record width is rejected during preparation");
    ok(context == NULL, "failed preparation clears the output pointer");
    if (context && context != (MMDB_lookup_context_s *)(uintptr_t)1)
        MMDB_free_lookup_context(context);

    /* A valid address range above the native-index budget must be rejected
     * before copying nodes. calloc normally keeps this source view lazy. */
    uint32_t count = (64U * 1024U * 1024U) / 8U + 1U;
    size_t bytes = (size_t)count * 6U + 18U;
    uint8_t *large = calloc(bytes, 1);
    if (!large)
        BAIL_OUT("could not allocate source view for index-budget test");
    mmdb = borrowed_reader(large, count, 6);
    context = (MMDB_lookup_context_s *)(uintptr_t)1;
    error = MMDB_create_lookup_context(&mmdb, &context);
    cmp_ok(error,
           "==",
           MMDB_OUT_OF_MEMORY_ERROR,
           "native index above 64 MiB is rejected before decoding");
    ok(context == NULL, "budget failure clears the output pointer");
    if (context && context != (MMDB_lookup_context_s *)(uintptr_t)1)
        MMDB_free_lookup_context(context);
    free(large);
    MMDB_free_lookup_context(NULL);
}

static MMDB_s reader_by_value(const char *path) {
    MMDB_s mmdb;
    if (MMDB_open(path, MMDB_MODE_MMAP, &mmdb) != MMDB_SUCCESS) {
        BAIL_OUT("could not open reader returned by value");
    }
    return mmdb;
}

static void check_reader_by_value(void) {
    char *path = test_database_path("MaxMind-DB-test-ipv4-24.mmdb");
    MMDB_s original = reader_by_value(path);
    free(path);
    MMDB_s moved = original;
    memset(&original, 0, sizeof(original));
    uint32_t state = 0x8b16a2cf;
    unsigned int mismatches = 0;
    for (unsigned int i = 0; i < 256; i++) {
        address_s query = make_query(i, &state);
        int expected_error, actual_error;
        MMDB_lookup_result_s expected =
            packed_sockaddr(&moved, &query.sa, &expected_error);
        MMDB_lookup_result_s actual =
            MMDB_lookup_sockaddr(&moved, &query.sa, &actual_error);
        if (!same_result(expected, expected_error, actual, actual_error)) {
            mismatches++;
        }
    }
    cmp_ok(
        mismatches, "==", 0, "automatic lookups use the moved reader address");
    MMDB_lookup_result_s result =
        lookup_string_ok(&moved, "1.1.1.1", "returned reader", "mmap mode");
    ok(result.entry.mmdb == &moved, "string result points to the moved reader");
    MMDB_entry_data_s data = data_ok(
        &result, MMDB_DATA_TYPE_UTF8_STRING, "returned entry", "ip", NULL);
    ok(data.has_data && data.type == MMDB_DATA_TYPE_UTF8_STRING &&
           data.data_size == 7 && !memcmp(data.utf8_string, "1.1.1.1", 7),
       "entry from a moved reader decodes normally");
    MMDB_close(&moved);
}

int main(void) {
    plan(NO_PLAN);
    for_all_record_sizes("MaxMind-DB-test-ipv4-%i.mmdb", run_database);
    for_all_record_sizes("MaxMind-DB-test-ipv6-%i.mmdb", run_database);
    for_all_record_sizes("MaxMind-DB-test-mixed-%i.mmdb", run_database);
    check_prefixes();
    check_cached_starts();
    check_corrupt_trees();
    check_creation_errors();
    check_reader_by_value();
    done_testing();
}
