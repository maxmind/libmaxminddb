#include "maxminddb_test_helper.h"

/* Keep the oracle on MMDB_read_node, whose public node decoding is separate
 * from the specialized address traversal. */
static MMDB_lookup_result_s reference_lookup(const MMDB_s *mmdb,
                                             const uint8_t *address,
                                             int family,
                                             int *error) {
    MMDB_lookup_result_s result = {.entry = {.mmdb = mmdb}};
    uint64_t value = 0;
    uint16_t bit = 0;
    if (family == AF_INET && mmdb->metadata.ip_version == 6) {
        value = mmdb->ipv4_start_node.node_value;
        bit = mmdb->ipv4_start_node.netmask;
    }
    *error = MMDB_SUCCESS;
    while (value < mmdb->metadata.node_count && bit < mmdb->depth) {
        MMDB_search_node_s node;
        *error = MMDB_read_node(mmdb, (uint32_t)value, &node);
        if (*error != MMDB_SUCCESS) {
            return result;
        }
        value = address[bit / 8] & (1U << (7 - bit % 8)) ? node.right_record
                                                         : node.left_record;
        bit++;
    }
    result.netmask = bit;
    result.found_entry = value > mmdb->metadata.node_count;
    if (result.found_entry) {
        result.entry.offset =
            (uint32_t)(value - mmdb->metadata.node_count - 16);
    }
    return result;
}

static uint32_t next_random(uint32_t *state) {
    *state ^= *state << 13;
    *state ^= *state >> 17;
    *state ^= *state << 5;
    return *state;
}

static void test_family(MMDB_s *mmdb, int family, const char *filename) {
    uint32_t state = 0x12345678;
    unsigned int mismatches = 0;
    unsigned int hits = 0;
    for (unsigned int i = 0; i < 4096; i++) {
        uint8_t address[16] = {0};
        for (unsigned int j = 0; j < sizeof(address); j++) {
            address[j] = (uint8_t)next_random(&state);
        }
        if (i % 2 == 0) {
            memset(address, 0, sizeof(address));
            if (family == AF_INET) {
                address[12] = address[13] = address[14] = 1;
                address[15] = (uint8_t)(i % 64);
            } else {
                address[11] = 2;
                address[15] = (uint8_t)(i % 128);
            }
        }
        struct sockaddr_in v4 = {.sin_family = AF_INET};
        struct sockaddr_in6 v6 = {.sin6_family = AF_INET6};
        memcpy(&v4.sin_addr, address + 12, 4);
        memcpy(&v6.sin6_addr, address, 16);
        const struct sockaddr *sockaddr = family == AF_INET
                                              ? (const struct sockaddr *)&v4
                                              : (const struct sockaddr *)&v6;
        const uint8_t *oracle_address =
            family == AF_INET && mmdb->metadata.ip_version == 4 ? address + 12
                                                                : address;
        if (family == AF_INET && mmdb->metadata.ip_version == 6) {
            memset(address, 0, 12);
        }
        int error;
        MMDB_lookup_result_s actual =
            MMDB_lookup_sockaddr(mmdb, sockaddr, &error);
        int expected_error;
        MMDB_lookup_result_s expected =
            reference_lookup(mmdb, oracle_address, family, &expected_error);
        if (error != expected_error ||
            actual.found_entry != expected.found_entry ||
            actual.netmask != expected.netmask || actual.entry.mmdb != mmdb ||
            (actual.found_entry &&
             actual.entry.offset != expected.entry.offset)) {
            mismatches++;
        }
        if (actual.found_entry) {
            hits++;
            const char *path[] = {"ip", NULL};
            MMDB_entry_data_s actual_data;
            MMDB_entry_data_s expected_data;
            int actual_status =
                MMDB_aget_value(&actual.entry, &actual_data, path);
            int expected_status =
                MMDB_aget_value(&expected.entry, &expected_data, path);
            if (actual_status != MMDB_SUCCESS ||
                actual_status != expected_status ||
                actual_data.type != MMDB_DATA_TYPE_UTF8_STRING ||
                actual_data.type != expected_data.type ||
                actual_data.data_size != expected_data.data_size ||
                memcmp(actual_data.utf8_string,
                       expected_data.utf8_string,
                       actual_data.data_size)) {
                mismatches++;
            }
        }
    }
    cmp_ok(mismatches,
           "==",
           0,
           "lookup, prefix and decoded values match node oracle - %s - IPv%d",
           filename,
           family == AF_INET ? 4 : 6);
    ok(hits > 0, "corpus includes successful traversals - %s", filename);
}

static void run_database(int UNUSED(record_size),
                         const char *filename,
                         const char *UNUSED(description)) {
    char *path = test_database_path(filename);
    MMDB_s *mmdb = open_ok(path, MMDB_MODE_MMAP, "mmap mode");
    free(path);
    if (!strstr(filename, "-ipv6-")) {
        test_family(mmdb, AF_INET, filename);
    }
    if (mmdb->metadata.ip_version == 6) {
        test_family(mmdb, AF_INET6, filename);
    }
    /* A shortened tree view must fail before any packed child is read. This
     * shallow copy borrows the open reader's mapping and is never closed. */
    MMDB_s truncated = *mmdb;
    truncated.flags |= MMDB_NO_PREPARE;
    truncated.data_section =
        truncated.file_content + truncated.full_record_byte_size - 1;
    struct sockaddr_in v4 = {.sin_family = AF_INET};
    struct sockaddr_in6 v6 = {.sin6_family = AF_INET6};
    const struct sockaddr *address = mmdb->metadata.ip_version == 4
                                         ? (const struct sockaddr *)&v4
                                         : (const struct sockaddr *)&v6;
    int error;
    (void)MMDB_lookup_sockaddr(&truncated, address, &error);
    cmp_ok(error,
           "==",
           MMDB_CORRUPT_SEARCH_TREE_ERROR,
           "truncated packed record fails before decoding - %s",
           filename);
    truncated.full_record_byte_size = 3;
    (void)MMDB_lookup_sockaddr(&truncated, address, &error);
    cmp_ok(error,
           "==",
           MMDB_UNKNOWN_DATABASE_FORMAT_ERROR,
           "unsupported packed record size is rejected - %s",
           filename);
    MMDB_close(mmdb);
    free(mmdb);
}

int main(void) {
    plan(NO_PLAN);
    for_all_record_sizes("MaxMind-DB-test-ipv4-%i.mmdb", &run_database);
    for_all_record_sizes("MaxMind-DB-test-ipv6-%i.mmdb", &run_database);
    for_all_record_sizes("MaxMind-DB-test-mixed-%i.mmdb", &run_database);
    done_testing();
}
