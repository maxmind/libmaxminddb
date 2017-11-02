#include "data-pool.h"
#include "maxminddb.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

static bool can_multiply(size_t const, size_t const, size_t const);

// Allocate an MMDB_data_pool_s. It initially has space for size
// MMDB_entry_data_list_s structs.
//
// The memory must be freed by the caller. You can choose to keep the memory in
// the data field around separately from the pool.
MMDB_data_pool_s *data_pool_new(size_t const size)
{
    if (size == 0 ||
        !can_multiply(SIZE_MAX, size, sizeof(MMDB_entry_data_list_s))) {
        return NULL;
    }

    MMDB_data_pool_s *const pool = calloc(1, sizeof(MMDB_data_pool_s));
    if (!pool) {
        return NULL;
    }

    pool->data = calloc(size, sizeof(MMDB_entry_data_list_s));
    if (!pool->data) {
        data_pool_destroy(pool);
        return NULL;
    }

    pool->size = size;
    pool->max_bytes = SIZE_MAX;
    pool->num_allocs = 1;

    return pool;
}

// Determine if we can multiply m*n. We can do this if the result will be below
// the given max. max will typically be SIZE_MAX.
//
// We want to know if we'll wrap around.
static bool can_multiply(size_t const max, size_t const m, size_t const n)
{
    if (m == 0) {
        return false;
    }

    return n <= max / m;
}

// Free an MMDB_data_pool_s.
void data_pool_destroy(MMDB_data_pool_s *const pool)
{
    if (!pool) {
        return;
    }

    if (pool->data) {
        free(pool->data);
    }

    free(pool);
}

// Retrieve a new struct from the pool. Doing this may cause the pool's size to
// grow.
MMDB_entry_data_list_s *data_pool_get(MMDB_data_pool_s *const pool)
{
    if (!pool || !pool->data) {
        return NULL;
    }

    if (pool->used_size < pool->size) {
        MMDB_entry_data_list_s *const list = pool->data + pool->used_size;
        pool->used_size++;
        return list;
    }

    // Grow.

    if (!can_multiply(SIZE_MAX, pool->size, 2)) {
        return NULL;
    }
    size_t const new_size = pool->size * 2;

    if (!can_multiply(pool->max_bytes, new_size,
                      sizeof(MMDB_entry_data_list_s))) {
        return NULL;
    }
    size_t const new_size_bytes = sizeof(MMDB_entry_data_list_s) * new_size;

    MMDB_entry_data_list_s *new_data = realloc(pool->data, new_size_bytes);
    if (!new_data) {
        return NULL;
    }
    pool->data = new_data;

    // It would be nice to zero the new memory. Doing that is expensive
    // however. We should not need to do it if we are careful about setting the
    // next pointers when we link up the list.

    pool->size = new_size;
    pool->num_allocs++;

    // Use the new space.

    MMDB_entry_data_list_s *const list = pool->data + pool->used_size;
    pool->used_size++;
    return list;
}

// Turn the structs in the array-like pool into a linked list.
MMDB_entry_data_list_s *data_pool_to_list(MMDB_data_pool_s *const pool)
{
    if (!pool || !pool->data || pool->used_size == 0) {
        return NULL;
    }

    for (size_t i = 0; i < pool->used_size - 1; i++) {
        MMDB_entry_data_list_s *const cur = pool->data + i;
        cur->next = pool->data + i + 1;
    }

    MMDB_entry_data_list_s *const last = pool->data + pool->used_size;
    last->next = NULL;

    return pool->data;
}

#ifdef TEST_DATA_POOL

#include <libtap/tap.h>
#include <maxminddb_test_helper.h>

static void test_can_multiply(void);

int main(void)
{
    plan(NO_PLAN);
    test_can_multiply();
    done_testing();
}

static void test_can_multiply(void)
{
    {
        ok(can_multiply(SIZE_MAX, 1, SIZE_MAX), "1*SIZE_MAX is ok");
    }

    {
        ok(!can_multiply(SIZE_MAX, 2, SIZE_MAX), "2*SIZE_MAX is not ok");
    }

    {
        ok(can_multiply(SIZE_MAX, 10240, sizeof(MMDB_entry_data_list_s)),
           "1024 entry_data_list_s's are okay");
    }
}

#endif
