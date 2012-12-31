

#ifndef GEOIP2_H
#define GEOIP2_H

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


# define GEOIP2_DTYPE_EXT (0)
# define GEOIP2_DTYPE_PTR (1)
# define GEOIP2_DTYPE_UTF8_STRING (2)
# define GEOIP2_DTYPE_DOUBLE (3)
# define GEOIP2_DTYPE_BYTES (4)
# define GEOIP2_DTYPE_UINT16 (5)
# define GEOIP2_DTYPE_UINT32 (6)
# define GEOIP2_DTYPE_HASH (7)
# define GEOIP2_DTYPE_INT32 (8)
# define GEOIP2_DTYPE_UINT64 (9)
# define GEOIP2_DTYPE_UINT128 (10)
# define GEOIP2_DTYPE_CONTAINER (11)
# define GEOIP2_DTYPE_END_MARKER (12)


/* GEOIP2 flags */
#define GEOIP2_MODE_STANDARD (1)
#define GEOIP2_MODE_MEMORY_CACHE (2)
#define GEOIP2_MODE_MASK (7)

/* GEOIP2 err codes */
#define GEOIP2_SUCCESS (0)
#define GEOIP2_OPENFILEERROR (-1)
#define GEOIP2_CORRUPTDATABASE (-2)
#define GEOIP2_INVALIDDATABASE (-3)
#define GEOIP2_IOERROR (-4)
#define GEOIP2_OUTOFMEMORY (-5)


struct GeoIP2_Lookup{
 U32 ptr;
 int netmask;
};

struct GeoIP2_Perl_Lookup {
  SV* sv;
  int netmask;
};

/* 128 bit address in network order */
typedef struct in6_addr geoipv6_t;


struct GeoIP2_Decode_Value {
  SV * sv;
  int new_offset;
};

struct GeoIP2_Decode_Key {
  const char * ptr;
  int size;
  int new_offset;
};

typedef struct GeoIP2 {
  U32 flags;
  int fd;
  const unsigned char * file_in_mem_ptr;
  const char * info;
  int file_format;
  int database_type;
  int minor_database_type;
  int recbits;
  int depth;
  int segments;
  const unsigned char * dataptr;
} GEOIP2_T;

unsigned int _lookup( struct GeoIP2 * gi, unsigned int ipnum );
void _decode( struct GeoIP2 * gi, int offset );

#ifdef __cplusplus
}
#endif

#endif /* GEOIP2_H */

