#ifndef MMDB_H
#define MMDB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
//#include <sys/socket.h>
#include <netinet/in.h>
//#include <arpa/inet.h>

// *** the EXT_TYPE is wrong it should be type - 8 not type
#define BROKEN_TYPE (1)
// every pointer start at the MAX + 1 of the previous type. To extend the range
// only th 32bit ptr contains all of them.
#define BROKEN_PTR (1)

// the searchtree ends the search unfortuately with offset zero instead of
// offset = segments.
#define BROKEN_SEARCHTREE (1)

# define MMDB_DTYPE_EXT (0)
# define MMDB_DTYPE_PTR (1)
# define MMDB_DTYPE_UTF8_STRING (2)
# define MMDB_DTYPE_DOUBLE (3)
# define MMDB_DTYPE_BYTES (4)
# define MMDB_DTYPE_UINT16 (5)
# define MMDB_DTYPE_UINT32 (6)
# define MMDB_DTYPE_HASH (7)
# define MMDB_DTYPE_INT32 (8)
# define MMDB_DTYPE_UINT64 (9)
# define MMDB_DTYPE_UINT128 (10)
# define MMDB_DTYPE_ARRAY (11)
# define MMDB_DTYPE_CONTAINER (12)
# define MMDB_DTYPE_END_MARKER (13)

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
    typedef struct MMDB_s {
        uint32_t flags;
        int fd;
        const uint8_t *file_in_mem_ptr;
        const char *info;
        int major_file_format;
        int minor_file_format;
        int database_type;
        int recbits;
        int depth;
        int segments;
        const uint8_t *dataptr;
    } MMDB_s;

    typedef struct MMDB_entry_s {
        MMDB_s *mmdb;
        unsigned int offset;    /* usually pointer to the struct */
        //uint8_t const *ptr;             /* usually pointer to the struct */
    } MMDB_entry_s;

    typedef struct {
        MMDB_entry_s entry;
        int netmask;
    } MMDB_root_entry_s;

    typedef struct MMDB_return_s {
        /* return values */
        union {
            double double_value;
            int sinteger;
            uint32_t uinteger;
            uint8_t c16[16];
            uint8_t const *ptr;
        };
        uint32_t offset;        /* start of our field or zero for not found */
        int data_size;          /* only valid for strings, utf8_strings or binary data */
        int type;               /* type like string utf8_string, int32, ... */
        int used_bytes;         /* real size of the value */
        int error;
    } MMDB_return_s;

    typedef struct MMDB_decode_s {
        MMDB_return_s data;
        uint32_t offset_to_next;
    } MMDB_decode_s;

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
    extern int MMDB_lookup_by_ipnum(uint32_t ipnum, MMDB_root_entry_s * res);
    extern int MMDB_lookup_by_ipnum_128(struct in6_addr ipnum,
                                        MMDB_root_entry_s * result);
    extern int MMDB_get_value(MMDB_entry_s * start, MMDB_return_s * result,
                              ...);
    extern int MMDB_strcmp_result(MMDB_s * mmdb,
                                  MMDB_return_s const *const result, char *str);

#ifdef __cplusplus
}
#endif
#endif                          /* MMDB_H */
