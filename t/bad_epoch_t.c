#include "maxminddb_test_helper.h"
#include "mmdb_test_writer.h"
#include <stdlib.h>

static void create_bad_epoch_db(const char *path) {
    uint32_t node_count = 1;
    uint32_t record_size = 24;
    uint32_t record_value = node_count + 16;

    size_t metadata_buf_size = 512;
    size_t data_size = 10;
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

    /* Data section: a simple map with one string entry */
    pos += write_map(file + pos, 1);
    pos += write_string(file + pos, "ip", 2);
    pos += write_string(file + pos, "test", 4);

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
    pos += write_uint64(file + pos, UINT64_MAX);

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

void test_bad_epoch(void) {
    const char *path = "/tmp/test_bad_epoch.mmdb";
    create_bad_epoch_db(path);

    /* Verify we can at least open the DB without crashing */
    MMDB_s mmdb;
    int status = MMDB_open(path, MMDB_MODE_MMAP, &mmdb);
    cmp_ok(status, "==", MMDB_SUCCESS, "opened bad-epoch MMDB");

    if (status != MMDB_SUCCESS) {
        diag("MMDB_open failed: %s", MMDB_strerror(status));
        remove(path);
        return;
    }

    /* Run mmdblookup --verbose via system() and check it doesn't crash.
     * We redirect output to /dev/null; the return code tells us
     * whether the process survived. Try both possible paths since tests
     * may run from either the project root or the t/ directory. */
    char cmd[512];
    const char *binary = "../bin/mmdblookup";
    FILE *test_bin = fopen(binary, "r");
    if (!test_bin) {
        binary = "./bin/mmdblookup";
        test_bin = fopen(binary, "r");
    }
    if (test_bin) {
        fclose(test_bin);
    }

    skip(!test_bin, 1, "mmdblookup binary not found");
    snprintf(cmd,
             sizeof(cmd),
             "%s -f %s -i 1.2.3.4 -v > /dev/null 2>&1",
             binary,
             path);
    int ret = system(cmd);
    /* system() returns the exit status; a signal-killed process gives
     * a non-zero status. WIFEXITED checks for normal exit. */
    ok(WIFEXITED(ret) && WEXITSTATUS(ret) == 0,
       "mmdblookup --verbose with UINT64_MAX build_epoch does not crash");
    end_skip;

    MMDB_close(&mmdb);
    remove(path);
}

int main(void) {
    plan(NO_PLAN);
    test_bad_epoch();
    done_testing();
}
