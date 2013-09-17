# NAME

libmaxminddb - a library for working with MaxMind DB files

# SYNOPSIS

    #include <maxminddb.h>
    
    int main(int argc, char **argv)
    {
        MMDB_s mmdb;
        int status = MMDB_open(fname, MMDB_MODE_MMAP, &mmdb);

        if (MMDB_SUCCESS != status) {
            fprintf(stderr, "\n  Can't open %s - %s\n", fname, MMDB_strerror(status));

            if (MMDB_IO_ERROR == status) {
                fprintf(stderr, "    IO error: %s\n", strerror(errno));
            }
            exit(1);
        }

        int gai_error, mmdb_error;
        MMDB_lookup_result_s result =
            MMDB_lookup_string(mmdb, ipstr, &gai_error, &mmdb_error);

        if (0 != gai_error) {
            fprintf(stderr,
                    "\n  Error from call to getaddrinfo for %s - %s\n\n",
                    ipstr, gai_strerror(gai_error));
            exit(2);
        }

        if (MMDB_SUCCESS != mmdb_error) {
            fprintf(stderr, "\n  Got an error from the maxminddb library: %s\n\n",
                    MMDB_strerror(mmdb_error));
            exit(3);
        }

        MMDB_entry_data_list_s *entry_data_list = NULL;

        int exit_code = 0;
        if (result.found_entry) {
            int status = MMDB_get_entry_data_list(&result.entry, &entry_data_list);

            if (MMDB_SUCCESS != status) {
                fprintf(stderr, "Got an error looking up the entry data - %s\n",
                        MMDB_strerror(status));
                exit_code = 4;
                goto end;
            }

            if (NULL != entry_data_list) {
                MMDB_dump_entry_data_list(stdout, entry_data_list, 2);
            }
        } else {
            fprintf(stderr,
                    "\n  Could not find an entry for this IP address (%s)\n\n",
                    ip_address);
            exit_code = 5;
        }

    end:
        MMDB_free_entry_data_list(entry_data_list);
        MMDB_close(&mmdb);
        exit(exit_code);
    }

# DESCRIPTION

The libmaxminddb library provides functions for working MaxMind DB files. The
database and results are all represented by different data
structures. Databases are opened by calling `MMDB_open()`. You can look up IP
addresses as a string with `MMDB_lookup_string()` or as a `struct sockaddr *`
with `MMDB_lookup_sockaddr()`.

If the lookup finds the IP address in the database, it returns a
`MMDB_lookup_result_s` struct. If that struct indicates that the database has
data for the IP, there are a number of functions that can be used to fetch
that data. These include `MMDB_get_value()` and
`MMDB_get_entry_data_list()`. See the function documentation below for more
details.

When you are done with the database handle you should call `MMDB_close()`.

All publicly visible functions, structures, and macros begin with "MMDB_".

# DATA STRUCTURES

All data structures exported by this library's `maxmindb.h` header are
typedef'd in the form `typedef struct foo_s { ... } foo_s` so you can refer to
them without the `struct` prefix.

This library provides the following data structures:

## `MMDB_s`

This is the handle for a MaxMind DB file. We only document some of this
structure's fields in order to discourage you from poking too deeply into this
handle!

    typedef struct MMDB_s {
        uint32_t flags;
        const char *filename;
        ...
        MMDB_metadata_s metadata;
    } MMDB_s;

* `uint32_t flags` - the flags this database was opened with. See the
  `MMDB_open()` documentation for more details.
* `const char *filename` - the name of the file which was opened, as passed to
  `MMDB_open()`.
* `MMDB_metadata_s metadata` - the metadata for the database.

## `MMDB_metadata_s` and `MMDB_description_s`

This struct can be retrieved from the `MMDB_s` struct. It contains the
metadata read from the database file. Note that you may find it more
convenient to access this metadata by calling
`MMDB_get_metadata_as_data_entry_list()` instead.

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

    typedef struct MMDB_description_s {
        const char *language;
        const char *description;
    } MMDB_description_s;

These structures should be mostly self-explanatory.

The `ip_version` member should always be `4` or `6`. The
`binary_format_major_version` should always be `2`.

There is no requirement that the database metadata include languages or
descriptions, so the `count` for these parts of the metadata can be zero. All
of the other `MMDB_metadata_s` fields should be populated.

## `MMDB_lookup_result_s`

This struct is returned as the result of looking up an IP address.

    typedef struct MMDB_lookup_result_s {
        bool found_entry;
        MMDB_entry_s entry;
        uint16_t netmask;
    } MMDB_lookup_result_s;

If the `found_entry` member is false then the other members of this struct do
not contain meaningful values. Always check that `found_entry` is true first!

The `entry` member is used to look up the data associated with the IP address.

The `netmask` member tells you what subnet the IP address belongs to in this
database. For example, if you look up the address `1.1.1.1` in an IPv4
database and the returned `netmask` is 16, then the address is part of the
`1.1.1.0/16` subnet.

## `MMDB_result_s`

You don't really need to dig around in this struct. You'll get this from a
`MMDB_lookup_result_s` struct and pass it to various functions.

## `MMDB_entry_data_s`

This struct is used to return a single data section entry for an IP. These
entries can in turn point to other entries, as is the case for things like
maps and arrays. Some members of this struct are not documented as they are
only for internal use.

    typedef struct MMDB_entry_data_s {
        union {
            uint32_t pointer;
            const char *utf8_string;
            double double_value;
            const uint8_t *bytes;
            uint16_t uint16;
            uint32_t uint32;
            int32_t int32;
            uint64_t uint64;
            {unsigned __int128 or uint8_t[16]} uint128;
            bool boolean;
            float float_value;
        };
        ...
        uint32_t data_size;
        uint32_t type;
    } MMDB_entry_data_s;

The union at the beginning of the struct defines the actual data. To determine
which union member is populated you should look at the `type` member. The
`pointer` member of the union should never be populated in any data returned
by the API. Pointers should always be resolved internally.

The handling of uint128 data depends on whether or not your platform supports
the `unsigned __int128` type. If it does, then the `uint128` member is of that
type. If that type is not supported you'll get back a 16 byte array of
`uint8_t` values instead. This is the raw data from the database.

The `data_size` member is only relevant for `utf8_string` and `bytes` data.

The `type` member can be compared to one of the `MMDB_DTYPE_*` macros.

### Pointer Values and `MMDB_close()`

The `utf8_string`, `bytes`, and (maybe) the `uint128` members of this struct
are all pointers directly into the database's data section. This can either be
a `malloc`'d or `mmap`'d block of memory. In either case, these pointers will
become invalid after `MMDB_close()` is called.

If you need to refer to this data after that time you should copy the data
with an appropriate function (`strdup`, `memcpy`, etc.).

## `MMDB_entry_data_list_s`

This structure encapsulates a linked list of `MMDB_entry_data_s` structs.

    typedef struct MMDB_entry_data_list_s {
        MMDB_entry_data_s entry_data;
        struct MMDB_entry_data_list_s *next;
    } MMDB_entry_data_list_s;

This structure lets you look at entire map or array data entry by iterating
over the linked list.

# STATUS CODES

This library returns (or populates) status codes for many functions. These
status codes are:

* `MMDB_SUCCESS` - yay, everything worked
* `MMDB_FILE_OPEN_ERROR` - there was an error trying to open the MaxMind DB
  file.
* `MMDB_IO_ERROR` - an IO operation failed. Check errno for more details.
* `MMDB_CORRUPT_SEARCH_TREE_ERROR` - looking up an IP address in the search
  tree gave us an impossible result. The database is probably damaged or was
  generated incorrectly.
* `MMDB_INVALID_METADATA_ERROR` - something in the database is wrong. This
  includes missing metadata keys as well as impossible values (like an
  `ip_version` of 7).
* `MMDB_UNKNOWN_DATABASE_FORMAT_ERROR` - The database metadata indicates that
  it's major version is not 2. This library can only handle major version 2.
* `MMDB_OUT_OF_MEMORY_ERROR` - a memory allocation call (`malloc`, etc.)
  failed.
* `MMDB_INVALID_DATA_ERROR` - an entry in the data section contains invalid
  data. For example, a uint16 field is claiming to be more than 2 bytes
  long. The database is probably damaged or was generated incorrectly.
* `MMDB_INVALID_LOOKUP_PATH` - The lookup path passed to `MMDB_get_value`,
  `MMDB_vget_value`, or `MMDB_aget_value` contains an array offset that is not
  a non-negative integer.

# FUNCTIONS

This library provides the following exported functions:

## `uint16_t MMDB_open(const char *filename, uint32_t flags, MMDB_s *mmdb)`

This function opens a handle to a MaxMind DB file. It's return value is a
status code as defined above. Always check this call's return value!

    MMDB_s mmdb;
    uint16_t status = MMDB_open("/path/to/file.mmdb", MMDB_MODE_MMAP, &mmdb);
    if (MMDB_SUCCESS != status) { ... }

The `MMDB_s` struct you pass in can be on the stack or allocated from the
heap.

The currently valid flags are:

* `MMDB_MODE_MMAP` - open the database with `mmap()`.
* `MMDB_MODE_MEMORY` - read the entire database into memory.

The "all in memory" mode is faster but uses more memory.

If you pass in some other value for the `flags` parameter who knows what will
happen. Don't do that. In the future we may add additional flags that you can
bitwise-or together with the mode, as well as additional modes.

## `MMDB_lookup_result_s MMDB_lookup_string(MMDB_s *mmdb, const char *ipstr, int *gai_error, int *mmdb_error)`

This function looks up an IP address that is passed by a string. Internally it
calls `getaddrinfo()` to resolve the address into a binary form. It then calls
`MMDB_lookup_sockaddr()` to look the address up in the database. If you have
already resolved an address you can call `MMDB_lookup_sockaddr()` directly,
rather than resolving the address twice.

    int gai_error, mmdb_error;
    MMDB_lookup_result_s result =
        MMDB_lookup_string(mmdb, "1.2.3.4", &gai_error, &mmdb_error);
    if (0 != gai_error) { ... }
    if (MMDB_SUCCESS != mmdb_error) { ... }
    
    if (result.found_entry) { ... }

This function always returns an `MMDB_lookup_result_s` struct, but you should
also check the `gai_error` and `mmdb_error` params. If either of these
indicates an error then the returned struct is meaningless.

If no error occurred you still need to make sure that the `found_entry` member
in the returned result is true. If it's not, this means that the IP address
does have an entry in the database.

This function will work with IPv4 addresses even when the database contains
data for both IPv4 and IPv6 addresses. The IPv4 address will be looked up as
is, it will not be remapped to the `::ffff:xxxx:xxxx` block allocated for
IPv4-mapped IPv6 addresses.

If you pass an IPv6 address to a database with only IPv4 data then the
`found_entry` member will be false, but the `mmdb_error` status will still be
`MMDB_SUCCESS`.

## MMDB_lookup_result_s MMDB_lookup_sockaddr(MMDB_s *mmdb, struct sockaddr *sockaddr, int *mmdb_error)

This function looks up an IP address that has already been resolved by
`getaddrinfo()`.

Other than not calling `getaddrinfo()` itself, this function is identical to
the `MMDB_lookup_string()` function.

    int mmdb_error;
    MMDB_lookup_result_s result =
        MMDB_lookup_sockaddr(mmdb, address->ai_addr, &mmdb_error);
    if (0 != gai_error) { ... }
    if (MMDB_SUCCESS != mmdb_error) { ... }
    
    if (result.found_entry) { ... }
