#ifndef MMDB_H
#define MMDB_H

#ifdef __cplusplus
extern "C" {
#endif
#define _GNU_SOURCE
#include <sys/types.h>
//#include <sys/socket.h>
#include <netinet/in.h>
//#include <arpa/inet.h>

#define MMDB_DEFAULT_DATABASE "/usr/local/share/GeoIP/GeoIP2-City.mmdb"

// *** the EXT_TYPE is wrong it should be type - 8 not type
#define BROKEN_TYPE (1)
// every pointer start at the MAX + 1 of the previous type. To extend the range
// only th 32bit ptr contains all of them.
#define BROKEN_PTR (0)

#define MMDB_DTYPE_EXT (0)
#define MMDB_DTYPE_PTR (1)
#define MMDB_DTYPE_UTF8_STRING (2)
#define MMDB_DTYPE_DOUBLE (3)
#define MMDB_DTYPE_BYTES (4)
#define MMDB_DTYPE_UINT16 (5)
#define MMDB_DTYPE_UINT32 (6)
#define MMDB_DTYPE_MAP (7)     /* HASH */
#define MMDB_DTYPE_INT32 (8)
#define MMDB_DTYPE_UINT64 (9)
#define MMDB_DTYPE_UINT128 (10)
#define MMDB_DTYPE_ARRAY (11)
#define MMDB_DTYPE_CONTAINER (12)
#define MMDB_DTYPE_END_MARKER (13)
#define MMDB_DTYPE_BOOLEAN (14)
#define MMDB_DTYPE_IEEE754_FLOAT (15)
#define MMDB_DTYPE_IEEE754_DOUBLE (16)

#define MMDB_DTYPE_MAX (MMDB_DTYPE_IEEE754_DOUBLE)

#define MMDB_DATASECTION_NOOP_SIZE (16)

/* GEOIPDB flags */
#define MMDB_MODE_STANDARD (1)
#define MMDB_MODE_MEMORY_CACHE (2)
#define MMDB_MODE_MASK (7)

/* GEOIPDB err codes */
#define MMDB_SUCCESS (0)
#define MMDB_OPENFILEERROR (-1)
#define MMDB_CORRUPTDATABASE (-2)
#define MMDB_INVALIDDATABASE (-3)
#define MMDB_IOERROR (-4)
#define MMDB_OUTOFMEMORY (-5)

/* Looks better */
#define MMDB_TRUE (1)
#define MMDB_FALSE (0)

/* */
#define MMDB_DEBUG (0)

#if MMDB_DEBUG
#define MMDB_DBG_CARP(...) fprintf(stderr, __VA_ARGS__ );
#define MMDB_DBG_ASSERT(ex) assert(#ex)
#else
#define MMDB_DBG_CARP(...)
#define MMDB_DBG_ASSERT(ex)
#endif

// This is the starting point for every search.
// It is like the hash to start the search. It may or may not the root hash
    typedef struct MMDB_entry_s {
        struct MMDB_s *mmdb;
        unsigned int offset;    /* usually pointer to the struct */
        //uint8_t const *ptr;             /* usually pointer to the struct */
    } MMDB_entry_s;

// the root entry is the entry structure from a lookup
// think of it as the root of all informations about the IP.

    typedef struct {
        MMDB_entry_s entry;
        int netmask;
    } MMDB_root_entry_s;

// information about the database file.
    typedef struct MMDB_s {
        uint32_t flags;
        int fd;
        const uint8_t *file_in_mem_ptr;
        int major_file_format;
        int minor_file_format;
        int database_type;
        uint32_t full_record_size_bytes;        /* recbits * 2 / 8 */
        int depth;
        int node_count;
        const uint8_t *dataptr;
        uint8_t *meta_data_content;
        struct MMDB_s *fake_metadata_db;
        MMDB_entry_s meta;      // should change to entry_s
    } MMDB_s;

// this is the result for every field
    typedef struct MMDB_return_s {
        /* return values */
        union {
            float float_value;
            double double_value;
            int sinteger;
            uint32_t uinteger;
            uint8_t c8[8];
            uint8_t c16[16];
            uint8_t const *ptr;
        };
        uint32_t offset;        /* start of our field or zero for not found */
        int data_size;          /* only valid for strings, utf8_strings or binary data */
        int type;               /* type like string utf8_string, int32, ... */
    } MMDB_return_s;

    // The decode structure is like the result ( return_s ) but with the start
    // of the next entry. For example if we search for a key but this is the
    // wrong key.
    typedef struct MMDB_decode_s {
        MMDB_return_s data;
        uint32_t offset_to_next;
    } MMDB_decode_s;

    typedef struct MMDB_decode_all_s {
        MMDB_decode_s decode;
        struct MMDB_decode_all_s *next;
        int indent;
    } MMDB_decode_all_s;

#if 0

    typedef struct MMDB_decode_key_s {
        unsigned int new_offset;
        unsigned int size;
        const uint8_t *ptr;
    } MMDB_decode_key_s;

    struct MMDB_Decode_Value {
        SV *sv;
        int new_offset;
    };
    struct MMDB_Decode_Key {
        const char *ptr;
        int size;
        int new_offset;
    };
#endif

    extern MMDB_s *MMDB_open(char *fname, uint32_t flags);
    extern void MMDB_close(MMDB_s * mmdb);
    extern int MMDB_lookup_by_ipnum(uint32_t ipnum, MMDB_root_entry_s * res);
    extern int MMDB_lookup_by_ipnum_128(struct in6_addr ipnum,
                                        MMDB_root_entry_s * result);
    extern int MMDB_get_value(MMDB_entry_s * start, MMDB_return_s * result,
                              ...);
    extern int MMDB_strcmp_result(MMDB_s * mmdb,
                                  MMDB_return_s const *const result, char *str);

    extern const char *MMDB_lib_version(void);

    extern int MMDB_dump(MMDB_decode_all_s * decode_all, int indent);
    extern int MMDB_get_tree(MMDB_entry_s * start,
                             MMDB_decode_all_s ** decode_all);
    extern MMDB_decode_all_s *MMDB_alloc_decode_all(void);
    extern void MMDB_free_decode_all(MMDB_decode_all_s * freeme);

    extern int MMDB_lookupaddressX(const char *host, int ai_family,
                                   int ai_flags, void *ip);

#ifdef __cplusplus
}
#endif
#endif                          /* MMDB_H */
