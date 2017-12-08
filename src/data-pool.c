#include "data-pool.h"
#include "maxminddb.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

static bool can_multiply(size_t const, size_t const, size_t const);
static MMDB_entry_data_list_s *data_pool_get_list_head(
    MMDB_entry_data_list_s *const);

// Allocate an MMDB_data_pool_s. It initially has space for size
// MMDB_entry_data_list_s structs.
int data_pool_new(size_t const size, MMDB_data_pool_s *const pool)
{
    if (!pool) {
        return -1;
    }

    if (size == 0 ||
        !can_multiply(SIZE_MAX, size, sizeof(MMDB_entry_data_list_s))) {
        data_pool_destroy(pool, false);
        return -1;
    }
    pool->size = size;
    pool->blocks[0] = calloc(pool->size, sizeof(MMDB_entry_data_list_s));
    if (!pool->blocks[0]) {
        data_pool_destroy(pool, false);
        return -1;
    }
    pool->blocks[0]->head = true;

    pool->sizes[0] = size;

    pool->block = pool->blocks[0];

    return 0;
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

// Clean up the data pool.
//
// You can keep the list elements around separate from the pool itself by
// passing keep_list as true. This is useful once you have no further use for
// the pool's management. If you do, you must clean up the list using
// data_pool_list_destroy().
void data_pool_destroy(MMDB_data_pool_s *const pool, bool const keep_list)
{
    if (!pool) {
        return;
    }

    if (!keep_list) {
        // It might be nice to use data_pool_list_destroy() so we have the same
        // path either way, but since we may not have linked up the list yet,
        // that doesn't make sense.
        for (size_t i = 0; i <= pool->index; i++) {
            free(pool->blocks[i]);
        }
    }
}

// Claim a new struct from the pool. Doing this may cause the pool's size to
// grow.
MMDB_entry_data_list_s *data_pool_alloc(MMDB_data_pool_s *const pool)
{
    if (!pool) {
        return NULL;
    }

    if (pool->used < pool->size) {
        MMDB_entry_data_list_s *const element = pool->block + pool->used;
        pool->used++;
        return element;
    }

    // Take it from a new block of memory.

    size_t const new_index = pool->index + 1;
    if (new_index == DATA_POOL_NUM_BLOCKS) {
        // See the comment about not growing this on DATA_POOL_NUM_BLOCKS.
        return NULL;
    }

    if (!can_multiply(SIZE_MAX, pool->size, 2)) {
        return NULL;
    }
    size_t const new_size = pool->size * 2;

    if (!can_multiply(SIZE_MAX, new_size, sizeof(MMDB_entry_data_list_s))) {
        return NULL;
    }
    pool->blocks[new_index] = calloc(new_size, sizeof(MMDB_entry_data_list_s));
    if (!pool->blocks[new_index]) {
        return NULL;
    }
    pool->blocks[new_index]->head = true;

    pool->index = new_index;
    pool->block = pool->blocks[pool->index];

    pool->size = new_size;
    pool->sizes[pool->index] = pool->size;

    MMDB_entry_data_list_s *const element = pool->block;
    pool->used = 1;
    return element;
}

// Destroy a linked list created using the data pool.
//
// This destroys only the linked list. This is useful if you used
// data_pool_destroy() but kept the list around.
//
// It is only valid to call this after linking up the list using
// data_pool_to_list() as it works by traversing the list.
bool data_pool_list_destroy(MMDB_entry_data_list_s *const list)
{
    // There are potentially multiple blocks of memory, each containing
    // multiple list elements. We need to free each block, not each element.

    MMDB_entry_data_list_s *element = list;
    while (element) {
        // We should always be at a head.
        if (!element->head) {
            return false;
        }

        MMDB_entry_data_list_s *const next_element = data_pool_get_list_head(
            element->next);
        free(element);
        element = next_element;
    }

    return true;
}

static MMDB_entry_data_list_s *data_pool_get_list_head(
    MMDB_entry_data_list_s *const list)
{
    MMDB_entry_data_list_s *element = list;
    while (element) {
        if (element->head) {
            return element;
        }

        element = element->next;
    }

    return NULL;
}

// Turn the structs in the array-like pool into a linked list.
//
// Before calling this function, the list isn't linked up.
MMDB_entry_data_list_s *data_pool_to_list(MMDB_data_pool_s *const pool)
{
    if (!pool) {
        return NULL;
    }

    if (pool->index == 0 && pool->used == 0) {
        return NULL;
    }

    for (size_t i = 0; i <= pool->index; i++) {
        MMDB_entry_data_list_s *const block = pool->blocks[i];

        size_t size = pool->sizes[i];
        if (i == pool->index) {
            size = pool->used;
        }

        for (size_t j = 0; j < size - 1; j++) {
            MMDB_entry_data_list_s *const cur = block + j;
            cur->next = block + j + 1;
        }

        if (i < pool->index) {
            MMDB_entry_data_list_s *const last = block + size - 1;
            last->next = pool->blocks[i + 1];
        }
    }

    return pool->blocks[0];
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
