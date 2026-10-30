/* Frozen ABI declarations from include/maxminddb.h at 8a4f947.
 * Keep these independent of the current public header. */
#ifndef MMDB_ABI_LEGACY_H
#define MMDB_ABI_LEGACY_H

#include "maxminddb_config.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #if defined(_MSC_VER)
        #define ssize_t SSIZE_T
    #endif
#else
    #include <netdb.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
#endif

#if !(MMDB_UINT128_IS_BYTE_ARRAY)
    #if MMDB_UINT128_USING_MODE
typedef unsigned int mmdb_uint128_t __attribute__((__mode__(TI)));
    #else
typedef unsigned __int128 mmdb_uint128_t;
    #endif
#endif

typedef struct MMDB_entry_s {
    const struct MMDB_s *mmdb;
    uint32_t offset;
} MMDB_entry_s;

typedef struct MMDB_lookup_result_s {
    bool found_entry;
    MMDB_entry_s entry;
    uint16_t netmask;
} MMDB_lookup_result_s;

typedef struct MMDB_entry_data_s {
    bool has_data;
    union {
        uint32_t pointer;
        const char *utf8_string;
        double double_value;
        const uint8_t *bytes;
        uint16_t uint16;
        uint32_t uint32;
        int32_t int32;
        uint64_t uint64;
#if MMDB_UINT128_IS_BYTE_ARRAY
        uint8_t uint128[16];
#else
        mmdb_uint128_t uint128;
#endif
        bool boolean;
        float float_value;
    };
    uint32_t offset;
    uint32_t offset_to_next;
    uint32_t data_size;
    uint32_t type;
} MMDB_entry_data_s;

typedef struct MMDB_description_s {
    const char *language;
    const char *description;
} MMDB_description_s;

typedef struct MMDB_metadata_s {
    uint32_t node_count;
    uint16_t record_size;
    uint16_t ip_version;
    const char *database_type;
    struct {
        size_t count;
        const char **names;
    } languages;
    uint16_t binary_format_major_version;
    uint16_t binary_format_minor_version;
    uint64_t build_epoch;
    struct {
        size_t count;
        MMDB_description_s **descriptions;
    } description;
} MMDB_metadata_s;

typedef struct MMDB_ipv4_start_node_s {
    uint16_t netmask;
    uint32_t node_value;
} MMDB_ipv4_start_node_s;

typedef struct MMDB_s {
    uint32_t flags;
    const char *filename;
    ssize_t file_size;
    const uint8_t *file_content;
    const uint8_t *data_section;
    uint32_t data_section_size;
    const uint8_t *metadata_section;
    uint32_t metadata_section_size;
    uint16_t full_record_byte_size;
    uint16_t depth;
    MMDB_ipv4_start_node_s ipv4_start_node;
    MMDB_metadata_s metadata;
} MMDB_s;

extern int MMDB_open(const char *filename, uint32_t flags, MMDB_s *mmdb);
extern MMDB_lookup_result_s MMDB_lookup_string(const MMDB_s *mmdb,
                                               const char *ipstr,
                                               int *gai_error,
                                               int *mmdb_error);
extern MMDB_lookup_result_s MMDB_lookup_sockaddr(const MMDB_s *mmdb,
                                                 const struct sockaddr *address,
                                                 int *mmdb_error);
extern int MMDB_get_value(MMDB_entry_s *entry, MMDB_entry_data_s *data, ...);
extern void MMDB_close(MMDB_s *mmdb);

#endif
