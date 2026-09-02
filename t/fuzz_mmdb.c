#include "maxminddb-compat-util.h"
#include "maxminddb.h"
#include <unistd.h>

#define kMinInputLength 2
#define kMaxInputLength (256 * 1024)

extern int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    int status;
    FILE *fp;
    MMDB_s mmdb;
    char filename[256];

    if (size < kMinInputLength || size > kMaxInputLength) {
        return 0;
    }

    snprintf(filename, sizeof(filename), "/tmp/libfuzzer.%d", getpid());

    fp = fopen(filename, "wb");
    if (!fp) {
        return 0;
    }

    fwrite(data, size, sizeof(uint8_t), fp);
    fclose(fp);

    status = MMDB_open(filename, MMDB_MODE_MMAP, &mmdb);
    if (status == MMDB_SUCCESS) {
        int gai_error, mmdb_error;
        MMDB_lookup_result_s result =
            MMDB_lookup_string(&mmdb, "1.1.1.1", &gai_error, &mmdb_error);
        if (gai_error == 0 && mmdb_error == MMDB_SUCCESS &&
            result.found_entry) {
            MMDB_entry_data_list_s *entry_data_list = NULL;
            MMDB_get_entry_data_list(&result.entry, &entry_data_list);
            MMDB_free_entry_data_list(entry_data_list);
        }
        MMDB_close(&mmdb);
    }

    unlink(filename);
    return 0;
}
