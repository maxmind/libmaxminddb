#ifndef IPDB_H
#define IPDB_H

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

# define IPDB_DTYPE_EXT (0)
# define IPDB_DTYPE_PTR (1)
# define IPDB_DTYPE_UTF8_STRING (2)
# define IPDB_DTYPE_DOUBLE (3)
# define IPDB_DTYPE_BYTES (4)
# define IPDB_DTYPE_UINT16 (5)
# define IPDB_DTYPE_UINT32 (6)
# define IPDB_DTYPE_HASH (7)
# define IPDB_DTYPE_INT32 (8)
# define IPDB_DTYPE_UINT64 (9)
# define IPDB_DTYPE_UINT128 (10)
# define IPDB_DTYPE_CONTAINER (11)
# define IPDB_DTYPE_END_MARKER (12)

/* GEOIPDB flags */
#define IPDB_MODE_STANDARD (1)
#define IPDB_MODE_MEMORY_CACHE (2)
#define IPDB_MODE_MASK (7)

/* GEOIPDB err codes */
#define IPDB_SUCCESS (0)
#define IPDB_OPENFILEERROR (-1)
#define IPDB_CORRUPTDATABASE (-2)
#define IPDB_INVALIDDATABASE (-3)
#define IPDB_IOERROR (-4)
#define IPDB_OUTOFMEMORY (-5)
	typedef struct IPDB {
		uint32_t flags;
		int fd;
		const unsigned char *file_in_mem_ptr;
		const char *info;
		int file_format;
		int database_type;
		int minor_database_type;
		int recbits;
		int depth;
		int segments;
		const unsigned char *dataptr;
	} IPDB;

	typedef struct IPDB_entry_s {
		GEOIPDB *gi;
		void *sptr;	/* usually pointer to the struct */
	} IPDB_entry_s;

	typedef struct {
		IPDB_entry_s entry;
		int netmask;
	} IPDB_root_entry_s;


#if 0
	struct IPDB_Decode_Value {
		SV *sv;
		int new_offset;
	};
	struct IPDB_Decode_Key {
		const char *ptr;
		int size;
		int new_offset;
	};
#endif

	unsigned int _lookup(GEOIPDB *gi, unsigned int ipnum);
	void _decode(GEOIPDB *gi, int offset);

#ifdef __cplusplus
}
#endif
#endif				/* IPDB_H */
