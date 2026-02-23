#include "maxminddb_test_helper.h"
#include "mmdb_test_writer.h"
#include <stdlib.h>

/*
 * Write a map control byte with a large size using case-31 encoding.
 * Type 7 (map) fits in the base types: control byte = (7 << 5) | size_marker.
 * For case-31 size encoding: control byte size bits = 31,
 * then 3 bytes for (size - 65821).
 */
static int write_large_map(uint8_t *buf, uint32_t size) {
    uint32_t adjusted = size - 65821;
    buf[0] = (7 << 5) | 31; /* type 7 (map), size = case 31 */
    buf[1] = (adjusted >> 16) & 0xff;
    buf[2] = (adjusted >> 8) & 0xff;
    buf[3] = adjusted & 0xff;
    return 4;
}

/*
 * Write an array control byte with a large size using case-31 encoding.
 * Type 11 (array) is extended: control byte = (0 << 5) | size_marker,
 * followed by extended type byte 4.
 * For case-31 size encoding: control byte size bits = 31,
 * then 3 bytes for (size - 65821).
 */
static int write_large_array(uint8_t *buf, uint32_t size) {
    uint32_t adjusted = size - 65821;
    buf[0] = (0 << 5) | 31; /* extended type, size = case 31 */
    buf[1] = 4;             /* extended type: 7 + 4 = 11 (array) */
    buf[2] = (adjusted >> 16) & 0xff;
    buf[3] = (adjusted >> 8) & 0xff;
    buf[4] = adjusted & 0xff;
    return 5;
}

/*
 * Create a crafted MMDB with an array claiming 1,000,000 elements but
 * only a few bytes of actual data.
 */
static void create_bad_data_size_db(const char *path) {
    uint32_t node_count = 1;
    uint32_t record_size = 24;
    uint32_t record_value = node_count + 16;

    /* The data section needs enough bytes for the array header + a few
     * elements but NOT enough for 1M elements. */
    size_t data_buf_size = 64;
    size_t metadata_buf_size = 512;
    size_t search_tree_size = 6;
    size_t total_size = search_tree_size + DATA_SEPARATOR_SIZE + data_buf_size +
                        METADATA_MARKER_LEN + metadata_buf_size;

    uint8_t *file = calloc(1, total_size);
    if (!file) {
        BAIL_OUT("calloc failed");
    }

    size_t pos = 0;

    /* Search tree: 1 node, 24-bit records, both pointing to data */
    file[pos++] = (record_value >> 16) & 0xff;
    file[pos++] = (record_value >> 8) & 0xff;
    file[pos++] = record_value & 0xff;
    file[pos++] = (record_value >> 16) & 0xff;
    file[pos++] = (record_value >> 8) & 0xff;
    file[pos++] = record_value & 0xff;

    /* 16-byte null separator */
    memset(file + pos, 0, DATA_SEPARATOR_SIZE);
    pos += DATA_SEPARATOR_SIZE;

    /* Data section: array claiming 1,000,000 elements */
    pos += write_large_array(file + pos, 1000000);

    /* Only write a few bytes of actual data (way less than 1M entries) */
    pos += write_string(file + pos, "x", 1);
    pos += write_string(file + pos, "y", 1);

    /* Pad to data_buf_size */
    size_t data_end = search_tree_size + DATA_SEPARATOR_SIZE + data_buf_size;
    if (pos < data_end) {
        pos = data_end;
    }

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
    file[pos++] = (0 << 5) | 0;
    file[pos++] = 4; /* extended type: 7 + 4 = 11 (array) */

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

void test_bad_data_size_rejected(void) {
    const char *path = "/tmp/test_bad_data_size.mmdb";
    create_bad_data_size_db(path);

    MMDB_s mmdb;
    int status = MMDB_open(path, MMDB_MODE_MMAP, &mmdb);
    cmp_ok(status, "==", MMDB_SUCCESS, "opened bad-data-size MMDB");

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
        MMDB_entry_data_list_s *entry_data_list = NULL;
        status = MMDB_get_entry_data_list(&result.entry, &entry_data_list);
        cmp_ok(status,
               "==",
               MMDB_INVALID_DATA_ERROR,
               "MMDB_get_entry_data_list returns INVALID_DATA_ERROR "
               "for array with size exceeding remaining data");
        MMDB_free_entry_data_list(entry_data_list);
    }

    MMDB_close(&mmdb);
    remove(path);
}

/*
 * Create a crafted MMDB with a map claiming 1,000,000 entries but
 * only a few bytes of actual data.
 */
static void create_bad_map_size_db(const char *path) {
    uint32_t node_count = 1;
    uint32_t record_size = 24;
    uint32_t record_value = node_count + 16;

    size_t data_buf_size = 64;
    size_t metadata_buf_size = 512;
    size_t search_tree_size = 6;
    size_t total_size = search_tree_size + DATA_SEPARATOR_SIZE + data_buf_size +
                        METADATA_MARKER_LEN + metadata_buf_size;

    uint8_t *file = calloc(1, total_size);
    if (!file) {
        BAIL_OUT("calloc failed");
    }

    size_t pos = 0;

    /* Search tree: 1 node, 24-bit records, both pointing to data */
    file[pos++] = (record_value >> 16) & 0xff;
    file[pos++] = (record_value >> 8) & 0xff;
    file[pos++] = record_value & 0xff;
    file[pos++] = (record_value >> 16) & 0xff;
    file[pos++] = (record_value >> 8) & 0xff;
    file[pos++] = record_value & 0xff;

    /* 16-byte null separator */
    memset(file + pos, 0, DATA_SEPARATOR_SIZE);
    pos += DATA_SEPARATOR_SIZE;

    /* Data section: map claiming 1,000,000 entries */
    pos += write_large_map(file + pos, 1000000);

    /* Only write a couple of key-value pairs (way less than 1M entries) */
    pos += write_string(file + pos, "k", 1);
    pos += write_string(file + pos, "v", 1);

    /* Pad to data_buf_size */
    size_t data_end = search_tree_size + DATA_SEPARATOR_SIZE + data_buf_size;
    if (pos < data_end) {
        pos = data_end;
    }

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
    file[pos++] = (0 << 5) | 0;
    file[pos++] = 4; /* extended type: 7 + 4 = 11 (array) */

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

void test_bad_map_size_rejected(void) {
    const char *path = "/tmp/test_bad_map_size.mmdb";
    create_bad_map_size_db(path);

    MMDB_s mmdb;
    int status = MMDB_open(path, MMDB_MODE_MMAP, &mmdb);
    cmp_ok(status, "==", MMDB_SUCCESS, "opened bad-map-size MMDB");

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
        MMDB_entry_data_list_s *entry_data_list = NULL;
        status = MMDB_get_entry_data_list(&result.entry, &entry_data_list);
        cmp_ok(status,
               "==",
               MMDB_INVALID_DATA_ERROR,
               "MMDB_get_entry_data_list returns INVALID_DATA_ERROR "
               "for map with size exceeding remaining data");
        MMDB_free_entry_data_list(entry_data_list);
    }

    MMDB_close(&mmdb);
    remove(path);
}

int main(void) {
    plan(NO_PLAN);
    test_bad_data_size_rejected();
    test_bad_map_size_rejected();
    done_testing();
}
