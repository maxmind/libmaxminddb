#include <data-pool.h>
#include "libtap/tap.h"
#include "maxminddb_test_helper.h"

static void test_data_pool_new(void);
static void test_data_pool_destroy(void);
static void test_data_pool_get(void);
static void test_data_pool_to_list(void);

int main(void)
{
    plan(NO_PLAN);
    test_data_pool_new();
    test_data_pool_destroy();
    test_data_pool_get();
    test_data_pool_to_list();
    done_testing();
}

static void test_data_pool_new(void)
{
    {
        MMDB_data_pool_s *const pool = data_pool_new(0);
        ok(pool == NULL, "size 0 is not valid");
    }

    {
        MMDB_data_pool_s *const pool = data_pool_new(SIZE_MAX - 10);
        ok(pool == NULL, "very large size is not valid");
    }

    {
        MMDB_data_pool_s *const pool = data_pool_new(512);
        ok(pool != NULL, "size 512 is valid");
        cmp_ok(pool->size, "==", 512, "size is 512");
        cmp_ok(pool->used_size, "==", 0, "used size is 0");
        cmp_ok(pool->num_allocs, "==", 1, "num allocs is 1");
        data_pool_destroy(pool);
    }
}

static void test_data_pool_destroy(void)
{
    {
        data_pool_destroy(NULL);
    }

    {
        MMDB_data_pool_s *const pool = data_pool_new(512);
        free(pool->data);
        pool->data = NULL;
        data_pool_destroy(pool);
    }
}

static void test_data_pool_get(void)
{
    {
        MMDB_data_pool_s *const pool = data_pool_new(1);
        ok(pool != NULL, "created pool");

        MMDB_entry_data_list_s *const entry1 = data_pool_get(pool);
        ok(entry1 != NULL, "got an entry");
        cmp_ok(pool->size, "==", 1, "pool size is still 1");
        cmp_ok(pool->used_size, "==", 1,
               "pool used size is 1 after taking one");
        cmp_ok(pool->num_allocs, "==", 1, "no new allocs yet");

        MMDB_entry_data_list_s *const entry2 = data_pool_get(pool);
        ok(entry2 != NULL, "got another entry");
        ok(entry1 != entry2, "second entry is different from first entry");
        cmp_ok(pool->size, "==", 2, "pool size is now 2");
        cmp_ok(pool->used_size, "==", 2,
               "pool used size is 2 after taking another");
        cmp_ok(pool->num_allocs, "==", 2, "a new alloc happened");

        data_pool_destroy(pool);
    }

    {
        size_t const initial_size = 10;
        MMDB_data_pool_s *const pool = data_pool_new(initial_size);
        ok(pool != NULL, "created pool");

        for (size_t i = 0; i < initial_size; i++) {
            MMDB_entry_data_list_s *const entry = data_pool_get(pool);
            ok(entry != NULL, "got an entry");
        }

        cmp_ok(pool->size, "==", initial_size, "pool size is the initial size");
        cmp_ok(pool->used_size, "==", 10, "used size is as expected");
        cmp_ok(pool->num_allocs, "==", 1, "no new allocs happened");

        MMDB_entry_data_list_s *const entry = data_pool_get(pool);
        ok(entry != NULL, "got an entry");

        cmp_ok(pool->size, "==", initial_size * 2,
               "pool size is the initial size*2");
        cmp_ok(pool->used_size, "==", 11, "used size is as expected");
        cmp_ok(pool->num_allocs, "==", 2, "new alloc happened");

        data_pool_destroy(pool);
    }

    {
        size_t const initial_size = 16;
        MMDB_data_pool_s *const pool = data_pool_new(initial_size);
        ok(pool != NULL, "created pool");

        pool->max_bytes = 16 * 2 * sizeof(MMDB_entry_data_list_s);

        for (size_t i = 0; i < initial_size * 2; i++) {
            MMDB_entry_data_list_s *const entry = data_pool_get(pool);
            ok(entry != NULL, "got entry %zu", i);
        }

        cmp_ok(pool->size, "==", 32, "pool is now size 32");
        cmp_ok(pool->used_size, "==", 32, "pool is using 32");
        cmp_ok(pool->num_allocs, "==", 2, "2 allocs");

        MMDB_entry_data_list_s *const entry = data_pool_get(pool);
        ok(entry == NULL, "did not get an entry, hit max");

        data_pool_destroy(pool);
    }
}

static void test_data_pool_to_list(void)
{
    {
        size_t const initial_size = 16;
        MMDB_data_pool_s *const pool = data_pool_new(initial_size);
        ok(pool != NULL, "created pool");

        MMDB_entry_data_list_s *const list_empty = data_pool_to_list(pool);
        ok(list_empty == NULL, "no list when no entries");

        MMDB_entry_data_list_s *const entry = data_pool_get(pool);
        ok(entry != NULL, "got an entry");

        MMDB_entry_data_list_s *const list_one_element =
            data_pool_to_list(pool);
        ok(list_one_element != NULL, "got a list");
        ok(list_one_element == entry,
           "list's first element is the first we retrieved");
        ok(list_one_element->next == NULL, "list is one element in size");

        MMDB_entry_data_list_s *const entry2 = data_pool_get(pool);
        ok(entry2 != NULL, "got another entry");

        MMDB_entry_data_list_s *const list_two_elements =
            data_pool_to_list(pool);
        ok(list_two_elements != NULL, "got a list");
        ok(list_two_elements == entry,
           "list's first element is the first we retrieved");
        ok(list_two_elements->next != NULL, "list has a second element");

        MMDB_entry_data_list_s *const second_element = list_two_elements->next;
        ok(second_element == entry2,
           "second item in list is second we retrieved");
        ok(second_element->next == NULL, "list ends with the second element");

        data_pool_destroy(pool);
    }

    {
        size_t const initial_size = 1;
        MMDB_data_pool_s *const pool = data_pool_new(initial_size);
        ok(pool != NULL, "created pool");

        MMDB_entry_data_list_s *const list_empty = data_pool_to_list(pool);
        ok(list_empty == NULL, "no list when no entries");

        MMDB_entry_data_list_s *const entry = data_pool_get(pool);
        ok(entry != NULL, "got an entry");

        MMDB_entry_data_list_s *const list_one_element =
            data_pool_to_list(pool);
        ok(list_one_element != NULL, "got a list");
        ok(list_one_element == entry,
           "list's first element is the first we retrieved");
        ok(list_one_element->next == NULL, "list ends with this element");

        data_pool_destroy(pool);
    }

    {
        size_t const initial_size = 2;
        MMDB_data_pool_s *const pool = data_pool_new(initial_size);
        ok(pool != NULL, "created pool");

        MMDB_entry_data_list_s *const list_empty = data_pool_to_list(pool);
        ok(list_empty == NULL, "no list when no entries");

        MMDB_entry_data_list_s *const entry1 = data_pool_get(pool);
        ok(entry1 != NULL, "got an entry");
        MMDB_entry_data_list_s *const entry2 = data_pool_get(pool);
        ok(entry2 != NULL, "got an entry");
        ok(entry1 != entry2, "second entry is different from the first");

        MMDB_entry_data_list_s *const list_element1 =
            data_pool_to_list(pool);
        ok(list_element1 != NULL, "got a list");
        ok(list_element1 == entry1,
           "list's first element is the first we retrieved");

        MMDB_entry_data_list_s *const list_element2 = list_element1->next;
        ok(list_element2 == entry2,
           "second element is the second we retrieved");
        ok(list_element2->next == NULL, "list ends with this element");

        data_pool_destroy(pool);
    }
}
