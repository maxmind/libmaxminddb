#ifndef MMDB_TEST_WRITER_H
#define MMDB_TEST_WRITER_H

/*
 * Shared helpers for tests that construct MMDB files in memory.
 * Each function writes one MMDB data-format element into buf and returns
 * the number of bytes written.
 */

#include <stdint.h>
#include <string.h>

/* MMDB binary format constants */
#define METADATA_MARKER "\xab\xcd\xefMaxMind.com"
#define METADATA_MARKER_LEN 14
#define DATA_SEPARATOR_SIZE 16

static inline int write_map(uint8_t *buf, uint32_t size) {
    buf[0] = (7 << 5) | (size & 0x1f);
    return 1;
}

static inline int write_string(uint8_t *buf, const char *str, uint32_t len) {
    buf[0] = (2 << 5) | (len & 0x1f);
    memcpy(buf + 1, str, len);
    return 1 + len;
}

static inline int write_uint16(uint8_t *buf, uint16_t value) {
    buf[0] = (5 << 5) | 2;
    buf[1] = (value >> 8) & 0xff;
    buf[2] = value & 0xff;
    return 3;
}

static inline int write_uint32(uint8_t *buf, uint32_t value) {
    buf[0] = (6 << 5) | 4;
    buf[1] = (value >> 24) & 0xff;
    buf[2] = (value >> 16) & 0xff;
    buf[3] = (value >> 8) & 0xff;
    buf[4] = value & 0xff;
    return 5;
}

static inline int write_uint64(uint8_t *buf, uint64_t value) {
    buf[0] = (0 << 5) | 8;
    buf[1] = 2; /* extended type: 7 + 2 = 9 (uint64) */
    buf[2] = (value >> 56) & 0xff;
    buf[3] = (value >> 48) & 0xff;
    buf[4] = (value >> 40) & 0xff;
    buf[5] = (value >> 32) & 0xff;
    buf[6] = (value >> 24) & 0xff;
    buf[7] = (value >> 16) & 0xff;
    buf[8] = (value >> 8) & 0xff;
    buf[9] = value & 0xff;
    return 10;
}

static inline int write_meta_key(uint8_t *buf, const char *key) {
    return write_string(buf, key, strlen(key));
}

#endif /* MMDB_TEST_WRITER_H */
