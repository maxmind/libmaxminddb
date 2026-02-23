#include "maxminddb_test_helper.h"
#include "mmdb_test_writer.h"
#include <stdlib.h>

/*
 * Create a crafted MMDB file with deeply nested maps and write it to path.
 * The data section contains: {"a": {"a": {"a": ... {"a": "x"} ...}}}
 * nested `depth` levels deep. All IP lookups resolve to this data.
 */
static void create_deep_nesting_db(const char *path, int depth) {
    uint32_t node_count = 1;
    uint32_t record_size = 24;
    uint32_t record_value = node_count + 16;

    size_t data_size = (size_t)depth * 3 + 2;
    size_t metadata_buf_size = 512;
    size_t search_tree_size = 6;
    size_t total_size = search_tree_size + DATA_SEPARATOR_SIZE + data_size +
                        METADATA_MARKER_LEN + metadata_buf_size;

    uint8_t *file = calloc(1, total_size);
    if (!file) {
        BAIL_OUT("calloc failed");
    }

    size_t pos = 0;

    /* Search tree: 1 node, 24-bit records */
    file[pos++] = (record_value >> 16) & 0xff;
    file[pos++] = (record_value >> 8) & 0xff;
    file[pos++] = record_value & 0xff;
    file[pos++] = (record_value >> 16) & 0xff;
    file[pos++] = (record_value >> 8) & 0xff;
    file[pos++] = record_value & 0xff;

    /* 16-byte null separator */
    memset(file + pos, 0, DATA_SEPARATOR_SIZE);
    pos += DATA_SEPARATOR_SIZE;

    /* Data section: deeply nested maps */
    for (int i = 0; i < depth; i++) {
        pos += write_map(file + pos, 1);
        pos += write_string(file + pos, "a", 1);
    }
    pos += write_string(file + pos, "x", 1);

    /* Metadata marker */
    memcpy(file + pos, METADATA_MARKER, METADATA_MARKER_LEN);
    pos += METADATA_MARKER_LEN;

    /* Metadata map */
    pos += write_map(file + pos, 9);

    pos += write_meta_key(file + pos, "binary_format_major_version");
    pos += write_uint16(file + pos, 2);

    pos += write_meta_key(file + pos, "binary_format_minor_version");
    pos += write_uint16(file + pos, 0);

    pos += write_meta_key(file + pos, "build_epoch");
    pos += write_uint64(file + pos, 1000000000ULL);

    pos += write_meta_key(file + pos, "database_type");
    pos += write_string(file + pos, "Test", 4);

    pos += write_meta_key(file + pos, "description");
    pos += write_map(file + pos, 0);

    pos += write_meta_key(file + pos, "ip_version");
    pos += write_uint16(file + pos, 4);

    pos += write_meta_key(file + pos, "languages");
    file[pos++] = (0 << 5) | 0; /* type 0 (extended), size 0 */
    file[pos++] = 4;            /* extended type: 7 + 4 = 11 (array) */

    pos += write_meta_key(file + pos, "node_count");
    pos += write_uint32(file + pos, node_count);

    pos += write_meta_key(file + pos, "record_size");
    pos += write_uint16(file + pos, record_size);

    FILE *f = fopen(path, "wb");
    if (!f) {
        free(file);
        BAIL_OUT("fopen failed");
    }
    if (fwrite(file, 1, pos, f) != pos) {
        fclose(f);
        free(file);
        BAIL_OUT("fwrite failed");
    }
    fclose(f);
    free(file);
}

void test_deep_nesting_rejected(void) {
    const char *path = "/tmp/test_max_depth.mmdb";
    /* 600 levels is above MAXIMUM_DATA_STRUCTURE_DEPTH (512) */
    create_deep_nesting_db(path, 600);

    MMDB_s mmdb;
    int status = MMDB_open(path, MMDB_MODE_MMAP, &mmdb);
    cmp_ok(status, "==", MMDB_SUCCESS, "opened deeply nested MMDB");

    if (status != MMDB_SUCCESS) {
        diag("MMDB_open failed: %s", MMDB_strerror(status));
        remove(path);
        return;
    }

    int gai_error, mmdb_error;
    MMDB_lookup_result_s result =
        MMDB_lookup_string(&mmdb, "1.2.3.4", &gai_error, &mmdb_error);

    cmp_ok(mmdb_error, "==", MMDB_SUCCESS, "lookup succeeded");
    ok(result.found_entry, "entry found");

    if (result.found_entry) {
        /* Looking up non-existent key "z" forces skip_map_or_array to
         * recurse through all 600 nesting levels. With the depth limit,
         * this should return MMDB_INVALID_DATA_ERROR instead of crashing. */
        MMDB_entry_data_s entry_data;
        const char *lookup_path[] = {"z", NULL};
        status = MMDB_aget_value(&result.entry, &entry_data, lookup_path);
        cmp_ok(status,
               "==",
               MMDB_INVALID_DATA_ERROR,
               "MMDB_aget_value returns MMDB_INVALID_DATA_ERROR for "
               "deeply nested data exceeding max depth");
    }

    MMDB_close(&mmdb);
    remove(path);
}

void test_valid_nesting_allowed(void) {
    const char *path = "/tmp/test_valid_depth.mmdb";
    /* 10 levels is well within MAXIMUM_DATA_STRUCTURE_DEPTH (512) */
    create_deep_nesting_db(path, 10);

    MMDB_s mmdb;
    int status = MMDB_open(path, MMDB_MODE_MMAP, &mmdb);
    cmp_ok(status, "==", MMDB_SUCCESS, "opened moderately nested MMDB");

    if (status != MMDB_SUCCESS) {
        diag("MMDB_open failed: %s", MMDB_strerror(status));
        remove(path);
        return;
    }

    int gai_error, mmdb_error;
    MMDB_lookup_result_s result =
        MMDB_lookup_string(&mmdb, "1.2.3.4", &gai_error, &mmdb_error);

    cmp_ok(mmdb_error, "==", MMDB_SUCCESS, "lookup succeeded");
    ok(result.found_entry, "entry found");

    if (result.found_entry) {
        /* Looking up non-existent key "z" at depth 10 should NOT
         * trigger the depth limit — it should return the normal
         * "path does not match" error. */
        MMDB_entry_data_s entry_data;
        const char *lookup_path[] = {"z", NULL};
        status = MMDB_aget_value(&result.entry, &entry_data, lookup_path);
        cmp_ok(status,
               "==",
               MMDB_LOOKUP_PATH_DOES_NOT_MATCH_DATA_ERROR,
               "MMDB_aget_value returns PATH_DOES_NOT_MATCH for "
               "valid nesting depth");
    }

    MMDB_close(&mmdb);
    remove(path);
}

int main(void) {
    plan(NO_PLAN);
    test_deep_nesting_rejected();
    test_valid_nesting_allowed();
    done_testing();
}
