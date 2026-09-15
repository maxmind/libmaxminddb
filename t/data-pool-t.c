#include "maxminddb_test_helper.h"
#include <assert.h>
#include <data-pool.h>
#include <inttypes.h>
#include <math.h>

static void test_data_pool_new(void **UNUSED(state));
static void test_data_pool_destroy(void **UNUSED(state));
static void test_data_pool_alloc(void **UNUSED(state));
static void test_data_pool_to_list(void **UNUSED(state));
static bool create_and_check_list(size_t const, size_t const);
static void check_block_count(MMDB_entry_data_list_s const *const,
                              size_t const);

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_data_pool_new),
        cmocka_unit_test(test_data_pool_destroy),
        cmocka_unit_test(test_data_pool_alloc),
        cmocka_unit_test(test_data_pool_to_list),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}

static void test_data_pool_new(void **UNUSED(state)) {
    {
        MMDB_data_pool_s *const pool = data_pool_new(0, 512);
        assert_true_desc(!pool, "size 0 is not valid");
    }

    {
        MMDB_data_pool_s *const pool = data_pool_new(SIZE_MAX - 10, SIZE_MAX);
        assert_true_desc(!pool, "very large size is not valid");
    }

    {
        MMDB_data_pool_s *const pool = data_pool_new(512, 1024);
        assert_true_desc(pool != NULL, "size 512 is valid");
        assert_int_equal_desc(pool->size, 512, "size is 512");
        assert_int_equal_desc(pool->used, 0, "used size is 0");
        assert_int_equal_desc(pool->capacity, 512, "capacity is 512");
        assert_int_equal_desc(pool->max_size, 1024, "maximum size is 1024");
        data_pool_destroy(pool);
    }

    {
        MMDB_data_pool_s *const pool = data_pool_new(512, 10);
        assert_true_desc(pool != NULL,
                         "maximum smaller than initial size is valid");
        assert_int_equal_desc(
            pool->size, 10, "initial size is clamped to maximum");
        assert_int_equal_desc(
            pool->capacity, 10, "capacity is clamped to maximum");
        data_pool_destroy(pool);
    }
}

static void test_data_pool_destroy(void **UNUSED(state)) {
    {
        data_pool_destroy(NULL);
    }

    {
        MMDB_data_pool_s *const pool = data_pool_new(512, 512);
        assert_true_desc(pool != NULL, "created pool");
        data_pool_destroy(pool);
    }
}

static void test_data_pool_alloc(void **UNUSED(state)) {
    {
        MMDB_data_pool_s *const pool = data_pool_new(1, 3);
        assert_true_desc(pool != NULL, "created pool");
        assert_int_equal_desc(pool->used, 0, "used size starts at 0");

        MMDB_entry_data_list_s *const entry1 = data_pool_alloc(pool);
        assert_true_desc(entry1 != NULL, "allocated first entry");
        // Arbitrary so that we can recognize it.
        entry1->entry_data.offset = (uint32_t)123;

        assert_int_equal_desc(pool->size, 1, "size is still 1");
        assert_int_equal_desc(pool->used, 1, "used size is 1 after taking one");

        MMDB_entry_data_list_s *const entry2 = data_pool_alloc(pool);
        assert_true_desc(entry2 != NULL, "got another entry");
        assert_true_desc(entry1 != entry2,
                         "second entry is different from first entry");

        assert_int_equal_desc(pool->size, 2, "size is 2 (new block)");
        assert_int_equal_desc(pool->used, 1, "used size is 1 in current block");

        MMDB_entry_data_list_s *const entry3 = data_pool_alloc(pool);
        assert_true_desc(entry3 != NULL, "got the final allowed entry");
        assert_true_desc(data_pool_alloc(pool) == NULL,
                         "allocation past maximum capacity is rejected");
        assert_int_equal_desc(
            pool->capacity, 3, "capacity does not exceed maximum");

        assert_true_desc(entry1->entry_data.offset == 123,
                         "accessing the original entry's memory is ok");

        data_pool_destroy(pool);
    }

    {
        size_t const initial_size = 10;
        MMDB_data_pool_s *const pool =
            data_pool_new(initial_size, initial_size * 3);
        assert_true_desc(pool != NULL, "created pool");

        MMDB_entry_data_list_s *entry1 = NULL;
        for (size_t i = 0; i < initial_size; i++) {
            MMDB_entry_data_list_s *const entry = data_pool_alloc(pool);
            assert_true_desc(entry != NULL, "got an entry");
            // Give each a unique number so we can check it.
            entry->entry_data.offset = (uint32_t)i;
            if (i == 0) {
                entry1 = entry;
            }
        }

        assert_int_equal_desc(
            pool->size, initial_size, "size is the initial size");
        assert_int_equal_desc(
            pool->used, initial_size, "used size is as expected");

        MMDB_entry_data_list_s *const entry = data_pool_alloc(pool);
        assert_true_desc(entry != NULL, "got an entry");
        entry->entry_data.offset = (uint32_t)initial_size;

        assert_int_equal_desc(
            pool->size, initial_size * 2, "size is the initial size*2");
        assert_int_equal_desc(pool->used, 1, "used size is as expected");

        MMDB_entry_data_list_s *const list = data_pool_to_list(pool);

        MMDB_entry_data_list_s *element = list;
        for (size_t i = 0; i < initial_size + 1; i++) {
            assert_true_desc(element->entry_data.offset == (uint32_t)i,
                             "found offset %" PRIu32 ", should have %zu",
                             element->entry_data.offset,
                             i);
            element = element->next;
        }

        assert_true_desc(
            entry1->entry_data.offset == (uint32_t)0,
            "accessing entry1's original memory is ok after growing the pool");

        data_pool_destroy(pool);
    }

    {
        size_t const maximum_size = 65536;
        MMDB_data_pool_s *const pool = data_pool_new(64, maximum_size);
        assert_true_desc(pool != NULL, "created a decoder-sized pool");
        for (size_t i = 0; i < maximum_size; i++) {
            MMDB_entry_data_list_s *const entry = data_pool_alloc(pool);
            assert(entry != NULL);
            (void)entry;
        }
        assert_int_equal_desc(
            pool->capacity,
            maximum_size,
            "final block is clamped to the remaining capacity");
        assert_int_equal_desc(
            pool->sizes[pool->index],
            64,
            "the clamped final block reserves only 64 entries");
        assert_true_desc(data_pool_alloc(pool) == NULL,
                         "decoder-sized pool refuses a 65,537th entry");
        data_pool_destroy(pool);
    }
}

static void test_data_pool_to_list(void **UNUSED(state)) {
    {
        size_t const initial_size = 16;
        MMDB_data_pool_s *const pool =
            data_pool_new(initial_size, initial_size);
        assert_true_desc(pool != NULL, "created pool");

        MMDB_entry_data_list_s *const entry1 = data_pool_alloc(pool);
        assert_true_desc(entry1 != NULL, "got an entry");

        MMDB_entry_data_list_s *const list_one_element =
            data_pool_to_list(pool);
        assert_true_desc(list_one_element != NULL, "got a list");
        assert_true_desc(list_one_element == entry1,
                         "list's first element is the first we retrieved");
        assert_true_desc(list_one_element->next == NULL,
                         "list is one element in size");

        MMDB_entry_data_list_s *const entry2 = data_pool_alloc(pool);
        assert_true_desc(entry2 != NULL, "got another entry");

        MMDB_entry_data_list_s *const list_two_elements =
            data_pool_to_list(pool);
        assert_true_desc(list_two_elements != NULL, "got a list");
        assert_true_desc(list_two_elements == entry1,
                         "list's first element is the first we retrieved");
        assert_true_desc(list_two_elements->next != NULL,
                         "list has a second element");

        MMDB_entry_data_list_s *const second_element = list_two_elements->next;
        assert_true_desc(second_element == entry2,
                         "second item in list is second we retrieved");
        assert_true_desc(second_element->next == NULL,
                         "list ends with the second element");

        data_pool_destroy(pool);
    }

    {
        size_t const initial_size = 1;
        MMDB_data_pool_s *const pool =
            data_pool_new(initial_size, initial_size);
        assert_true_desc(pool != NULL, "created pool");

        MMDB_entry_data_list_s *const entry1 = data_pool_alloc(pool);
        assert_true_desc(entry1 != NULL, "got an entry");

        MMDB_entry_data_list_s *const list_one_element =
            data_pool_to_list(pool);
        assert_true_desc(list_one_element != NULL, "got a list");
        assert_true_desc(list_one_element == entry1,
                         "list's first element is the first we retrieved");
        assert_true_desc(list_one_element->next == NULL,
                         "list ends with this element");

        data_pool_destroy(pool);
    }

    {
        size_t const initial_size = 2;
        MMDB_data_pool_s *const pool =
            data_pool_new(initial_size, initial_size);
        assert_true_desc(pool != NULL, "created pool");

        MMDB_entry_data_list_s *const entry1 = data_pool_alloc(pool);
        assert_true_desc(entry1 != NULL, "got an entry");

        MMDB_entry_data_list_s *const entry2 = data_pool_alloc(pool);
        assert_true_desc(entry2 != NULL, "got an entry");
        assert_true_desc(entry1 != entry2,
                         "second entry is different from the first");

        MMDB_entry_data_list_s *const list_element1 = data_pool_to_list(pool);
        assert_true_desc(list_element1 != NULL, "got a list");
        assert_true_desc(list_element1 == entry1,
                         "list's first element is the first we retrieved");

        MMDB_entry_data_list_s *const list_element2 = list_element1->next;
        assert_true_desc(list_element2 == entry2,
                         "second element is the second we retrieved");
        assert_true_desc(list_element2->next == NULL,
                         "list ends with this element");

        data_pool_destroy(pool);
    }

    {
        print_message("starting test: fill one block save for one spot\n");
        assert_true_desc(create_and_check_list(3, 2),
                         "fill one block save for one spot");
    }

    {
        print_message("starting test: fill one block\n");
        assert_true_desc(create_and_check_list(3, 3), "fill one block");
    }

    {
        print_message("starting test: fill one block and use one spot in the "
                      "next block\n");
        assert_true_desc(create_and_check_list(3, 3 + 1),
                         "fill one block and use one spot in the next block");
    }

    {
        print_message("starting test: fill two blocks save for one spot\n");
        assert_true_desc(create_and_check_list(3, 3 + 3 * 2 - 1),
                         "fill two blocks save for one spot");
    }

    {
        print_message("starting test: fill two blocks\n");
        assert_true_desc(create_and_check_list(3, 3 + 3 * 2),
                         "fill two blocks");
    }

    {
        print_message(
            "starting test: fill two blocks and use one spot in the next\n");
        assert_true_desc(create_and_check_list(3, 3 + 3 * 2 + 1),
                         "fill two blocks and use one spot in the next");
    }

    {
        print_message("starting test: fill three blocks save for one spot\n");
        assert_true_desc(create_and_check_list(3, 3 + 3 * 2 + 3 * 2 * 2 - 1),
                         "fill three blocks save for one spot");
    }

    {
        print_message("starting test: fill three blocks\n");
        assert_true_desc(create_and_check_list(3, 3 + 3 * 2 + 3 * 2 * 2),
                         "fill three blocks");
    }

    // It would be nice to have a larger number of these, but it's expensive to
    // run many. We currently hardcode what this will be anyway, so varying
    // this is not very interesting.
    size_t const initial_sizes[] = {1, 2, 32, 64, 128, 256};

    size_t const max_element_count = 4096;

    for (size_t i = 0; i < sizeof(initial_sizes) / sizeof(initial_sizes[0]);
         i++) {
        size_t const initial_size = initial_sizes[i];

        for (size_t element_count = 0; element_count < max_element_count;
             element_count++) {
            assert(create_and_check_list(initial_size, element_count));
        }
    }
}

// Use assert() rather than cmocka as cmocka is significantly slower and we run
// this frequently.
static bool create_and_check_list(size_t const initial_size,
                                  size_t const element_count) {
    size_t max_size = initial_size;
    if (element_count > initial_size) {
        max_size = element_count;
    }
    MMDB_data_pool_s *const pool = data_pool_new(initial_size, max_size);
    assert(pool != NULL);

    assert(pool->used == 0);

    // Hold on to the pointers as we initially see them so that we can check
    // they are still valid after building the list.
    MMDB_entry_data_list_s **const entry_array =
        calloc(element_count, sizeof(MMDB_entry_data_list_s *));
    assert(entry_array != NULL);

    for (size_t i = 0; i < element_count; i++) {
        MMDB_entry_data_list_s *const entry = data_pool_alloc(pool);
        assert(entry != NULL);

        entry->entry_data.offset = (uint32_t)i;

        entry_array[i] = entry;
    }

    MMDB_entry_data_list_s *const list = data_pool_to_list(pool);

    if (element_count == 0) {
        assert(list == NULL);
        data_pool_destroy(pool);
        free(entry_array);
        return true;
    }

    assert(list != NULL);

    MMDB_entry_data_list_s *element = list;
    for (size_t i = 0; i < element_count; i++) {
        assert(element->entry_data.offset == (uint32_t)i);

        assert(element == entry_array[i]);

        element = element->next;
    }
    assert(element == NULL);

    check_block_count(list, initial_size);

    data_pool_destroy(pool);
    free(entry_array);
    return true;
}

// Use assert() rather than cmocka as cmocka is significantly slower and we run
// this frequently.
static void check_block_count(MMDB_entry_data_list_s const *const list,
                              size_t const initial_size) {
    size_t got_block_count = 0;
    size_t got_element_count = 0;

    MMDB_entry_data_list_s const *element = list;
    while (element) {
        got_element_count++;

        if (element->pool) {
            got_block_count++;
        }

        element = element->next;
    }

    // Because <number of elements> = <initial size> * 2^(number of blocks)
    double const a = ceil((double)got_element_count / (double)initial_size);
    double const b = log2(a);
    size_t const expected_block_count = ((size_t)b) + 1;

    assert(got_block_count == expected_block_count);
}
