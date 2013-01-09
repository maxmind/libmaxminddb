#ifndef MMDB_H
#define MMDB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
//#include <sys/socket.h>
#include <netinet/in.h>
//#include <arpa/inet.h>

//#include<stdio.h>
//#include<stdlib.h>
//#include<string.h>
//#include <sys/types.h> /* for fstat */
//#include <sys/stat.h>   /* for fstat */

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
        int file_format;
        int database_type;
        int minor_database_type;
        int recbits;
        int depth;
        int segments;
        const uint8_t *dataptr;
    } MMDB_s;

    typedef struct MMDB_entry_s {
        MMDB_s *ipdb;
        unsigned int offset;    /* usually pointer to the struct */
        //uint8_t const *ptr;             /* usually pointer to the struct */
    } MMDB_entry_s;

    typedef struct {
        MMDB_entry_s entry;
        int netmask;
    } MMDB_root_entry_s;

    typedef struct MMDB_decode_key_s {
        unsigned int new_offset;
        unsigned int size;
        const uint8_t *ptr;
    } MMDB_decode_key_s;

    typedef struct MMDB_return_s {
        /* return values */
        union {
            double double_value;
            uint32_t uinteger;
            uint8_t const *ptr;
            int data_size;      /* only valid for strings, utf8_strings or binary data */
        };
        int type;               /* type like string utf8_string, int32, ... */
        int used_bytes;         /* real size of the value */
        int error;
    } MMDB_return_s;

#if 0
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

#ifdef __cplusplus
}
#endif
#endif                          /* MMDB_H */
