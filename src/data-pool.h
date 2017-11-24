#ifndef DATA_POOL_H
#define DATA_POOL_H

#include "maxminddb.h"
#include <stddef.h>

// A pool of memory for MMDB_entry_data_list_s structs. This is so we can
// allocate multiple up front rather than one at a time. It gives performance
// benefits.
//
// You can think of the pool as an array. The order you add elements to it (by
// calling data_pool_get()) ends up as the order of the list you create with
// data_pool_to_list().
//
// The memory only grows. There is no support for releasing an element you take
// back to the pool.
typedef struct MMDB_data_pool_s {
    // The pool of memory for structs. The location of the memory is not stable
    // as we use realloc() to grow it. As such, the next pointers are not
    // useful until you call data_pool_to_list().
    MMDB_entry_data_list_s *data;

    // The size of the pool, counting by structs.
    size_t size;

    // The number of structs in use in the pool.
    size_t used_size;

    // The maximum amount of memory we'll allocate. This defaults to SIZE_MAX.
    // It exists so we can test hitting this.
    size_t max_bytes;

    // Keep track of how many times we allocate memory for the pool. This is an
    // aid for introspection.
    unsigned int num_allocs;
} MMDB_data_pool_s;

MMDB_data_pool_s *data_pool_new(size_t const);
void data_pool_destroy(MMDB_data_pool_s *const);
int data_pool_alloc(MMDB_data_pool_s *const, size_t *const);
MMDB_entry_data_list_s *data_pool_lookup(
    MMDB_data_pool_s const *const,
    size_t const
    );
MMDB_entry_data_list_s *data_pool_to_list(MMDB_data_pool_s *const);

#endif
