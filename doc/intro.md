
## Public API ##

    MMDB_open
    MMDB_close
    MMDB_resolve_address
    MMDB_lookup_by_ipnum_128
    MMDB_lookup_by_ipnum
    MMDB_get_tree
    MMDB_free_decode_all
    MMDB_pread

## Getting started ##

`maxminddb.h` contains all public API functions and definitions. Start with
`#include <maxminddb.h>`.

It is recommended to browse the example CAPI for python in the pydemo directory.
I would start with `MMDB_lookup_Py` in `py_libmaxmind.c` and go from there,

Use `python setup.py install` to install the code and ` python test.py` to run it.

## Function reference ##

### `MMDB_s *MMDB_open(char *fname, uint32_t flags)` ###

Open takes two arguments, the database filename typically xyz,mmdb and the operation mode.
We support curently only two modes, the diskbased `MMDB_MODE_STANDARD` and the in memory mode `MMDB_MODE_MEMORY_CACHE`.
`MMDB_MODE_STANDARD` is slower, but does not need much memory.

The structure `MMDB_s` contains all information to search the database file. Please consider all fields readonly.

### `void MMDB_close(MMDB_s * mmdb)` ###

Free's all memory associated with the database and the filehandle.

### `int MMDB_resolve_address(const char *addr, int ai_family, int ai_flags, void *ip)` ###

Resolves the IP address addr into ip. ip is a pointer to `in_addr` or `in6_addr` depend on your inputs.

For IPv4 it is probably 

    status = MMDB_resolve_address( addr, AF_INET, 0, ip );

If your database is IPv6 you should use

    status = MMDB_resolve_address( addr, AF_INET6, AI_V4MAPPED, ip6 );

So any numeric input works like 24.24.24.24, ::24.24.24.24, ::ffff:24.24.24.24 or 2001:4860:b002::68

The return value is gaierr ( man getaddrinfo ) or 0 on success.

### `int MMDB_lookup_by_ipnum_128(struct in6_addr ipnum, MMDB_lookup_result_s * result)` ###

The `MMDB_lookup_by_ipnum_128` checks if the ipnumber usually created with a call to `MMDB_resolve_address` is part of the database.

    // initialize the root entry structure with the database
    MMDB_lookup_result_s root {.entry.mmdb = mmdb };
    status = MMDB_lookup_by_ipnum_128(ip.v6, &root);
    if ( status == MMDB_SUCCESS ) {
        if (root.entry.offset ){
           // found something in the database
        }
    }

The example initilsize our search structure with the database. Reuse the stucture if you want.
The next line search the database for the ipnum. The result is always `MMDB_SUCCESS` unless somthing wired happened. Like out of memory or an io error.
Note, that even a failed search return `MMDB_SUCCESS`.

`entry.offset > 0` indicates, that we found something.

### `int MMDB_get_tree(entry_s * entry, MMDB_decode_all_s ** dec)` ###

`MMDB_get_tree` preparse the database content into smaller easy peaces.
Iterate over the `MMDB_decode_all_s` structure after this call.
**Please notice**, you have to free the `MMDB_decode_all_s` structure 
with `MMDB_free_decode_all` for every successful call to `MMDB_get_tree`.

The calling sequene looks like

    MMDB_decode_all_s *decode_all;
    status = MMDB_get_tree(&root.entry, &decode_all);
    if ( status == MMDB_SUCCESS ){
       // do something
       MMDB_free_decode_all(decode_all);       
    }

 The `MMDB_decode_all_s` structure contains _one_ decoded entry, and an internal offset
 to the next entry to decode. This offset is only useful for the decode functions.

 The `MMDB_entry_data_s` structure inside `MMDB_decode_s` is much more interesting, 
 it contains the almost decoded data for all data types returning something.

    MMDB_DATA_TYPE_UTF8_STRING
    MMDB_DATA_TYPE_IEEE754_DOUBLE
    MMDB_DATA_TYPE_BYTES
    MMDB_DATA_TYPE_UINT16
    MMDB_DATA_TYPE_UINT32
    MMDB_DATA_TYPE_MAP
    MMDB_DATA_TYPE_INT32
    MMDB_DATA_TYPE_UINT64
    MMDB_DATA_TYPE_UINT128
    MMDB_DATA_TYPE_ARRAY
    MMDB_DATA_TYPE_BOOLEAN
    MMDB_DATA_TYPE_IEEE754_FLOAT

`MMDB_entry_data_s` looks like:

    struct MMDB_entry_data_s {
        /* return values */
        union {
            float float_value;
            double double_value;
            int sinteger;
            uint32_t uinteger;
            uint8_t c8[8];
            uint8_t c16[16];
	    const void* ptr;
        };
        uint32_t offset;
        int data_size;
        int type;
    };

The sturcture is valid whenever `MMDB_entry_data_s.offset > 0`.
`MMDB_entry_data_s.type` contains the type from above for example `MMDB_DATA_TYPE_INT32`.
and the value of the field. Sometimes the size, if it makes sense ( for `MMDB_DATA_TYPE_BYTES`,`MMDB_DATA_TYPE_UTF8_STRING`, `MMDB_DATA_TYPE_ARRAY` and `MMDB_DATA_TYPE_MAP` ).


#### `MMDB_DATA_TYPE_UTF8_STRING`

    type: MMDB_DATA_TYPE_UTF8_STRING
    data_size: is the length of the string in bytes
    ptr: is a pointer to the string in memory, or for diskbased databases an offset into the datasection of the file.

#### `MMDB_DATA_TYPE_IEEE754_DOUBLE`

    type: MMDB_DATA_TYPE_IEEE754_DOUBLE
    double_value: contains the value

#### `MMDB_DATA_TYPE_BYTES`
    
    type: MMDB_DATA_TYPE_BYTES
    data_size:  Length in bytes
    ptr: is a pointer to the string in memory, or for diskbased databases an offset into the datasection of the file.

#### `MMDB_DATA_TYPE_UINT16`

    type: MMDB_DATA_TYPE_UINT16
    sinteger: contains the value

#### `MMDB_DATA_TYPE_UINT32`

    type: MMDB_DATA_TYPE_UINT32
    uinteger: contains the value

#### `MMDB_DATA_TYPE_INT32`

    type: MMDB_DATA_TYPE_INT32
    sinteger: contains the value

#### `MMDB_DATA_TYPE_INT64`

    type: MMDB_DATA_TYPE_UINT64
    c8: contains eight bytes in network order

#### `MMDB_DATA_TYPE_INT128`

    type: MMDB_DATA_TYPE_UINT128
    c16: contains 16 bytes in network order

#### `MMDB_DATA_TYPE_BOOLEAN`
    type: MMDB_DATA_TYPE_BOOLEAN
    sinteger: contains the value
    
#### `MMDB_DATA_TYPE_MAP`
    type: MMDB_DATA_TYPE_MAP
    data_size:  count key/value pairs in the dict

#### `MMDB_DATA_TYPE_ARRAY`
    type: MMDB_DATA_TYPE_ARRAY
    data_size:  Length of the array

#### `MMDB_DATA_TYPE_IEEE754_FLOAT`
    type: MMDB_DATA_TYPE_IEEE754_FLOAT
    double_value: contains the value


### `void MMDB_free_decode_all(MMDB_decode_all_s * dec)` ###

Free all temporary used memory by `MMDB_decode_all_s` typical used after `MMDB_get_tree`

### `int MMDB_pread(int fd, uint8_t * buffer, ssize_t to_read, off_t offset)` ###

MMDB_pread makes it easier for the user to read X bytes into a buffer,

The return value is `MMDB_SUCCESS` for SUCCESS or `MMDB_IOERROR` on failure.


