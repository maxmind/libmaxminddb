#ifndef MMIPDB_H
#define MMIPDB_H

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

# define MMIPDB_DTYPE_EXT (0)
# define MMIPDB_DTYPE_PTR (1)
# define MMIPDB_DTYPE_UTF8_STRING (2)
# define MMIPDB_DTYPE_DOUBLE (3)
# define MMIPDB_DTYPE_BYTES (4)
# define MMIPDB_DTYPE_UINT16 (5)
# define MMIPDB_DTYPE_UINT32 (6)
# define MMIPDB_DTYPE_HASH (7)
# define MMIPDB_DTYPE_INT32 (8)
# define MMIPDB_DTYPE_UINT64 (9)
# define MMIPDB_DTYPE_UINT128 (10)
# define MMIPDB_DTYPE_CONTAINER (11)
# define MMIPDB_DTYPE_END_MARKER (12)

/* GEOIPDB flags */
#define MMIPDB_MODE_STANDARD (1)
#define MMIPDB_MODE_MEMORY_CACHE (2)
#define MMIPDB_MODE_MASK (7)

/* GEOIPDB err codes */
#define MMIPDB_SUCCESS (0)
#define MMIPDB_OPENFILEERROR (-1)
#define MMIPDB_CORRUPTDATABASE (-2)
#define MMIPDB_INVALIDDATABASE (-3)
#define MMIPDB_IOERROR (-4)
#define MMIPDB_OUTOFMEMORY (-5)
    typedef struct MMIPDB_s {
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
    } MMIPDB_s;

    typedef struct MMIPDB_entry_s {
        MMIPDB_s *ipdb;
        unsigned int offset;             /* usually pointer to the struct */
        //uint8_t const *ptr;             /* usually pointer to the struct */
    } MMIPDB_entry_s;

    typedef struct {
        MMIPDB_entry_s entry;
        int netmask;
    } MMIPDB_root_entry_s;

    typedef struct MMIPDB_decode_key_s {
        unsigned int new_offset;
        unsigned int size;
        uint8_t *ptr;
    } MMIPDB_decode_key_s;

#if 0
    struct MMIPDB_Decode_Value {
        SV *sv;
        int new_offset;
    };
    struct MMIPDB_Decode_Key {
        const char *ptr;
        int size;
        int new_offset;
    };
#endif

#ifdef __cplusplus
}
#endif
#endif                          /* MMIPDB_H */
