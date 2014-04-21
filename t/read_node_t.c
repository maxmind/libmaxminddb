#include "maxminddb_test_helper.h"

void run_read_node_tests(MMDB_s *mmdb, const uint32_t tests[][3],
                         int test_count,
                         uint8_t record_size)
{
    for (int i = 0; i < test_count; i++) {
        uint32_t node_number = tests[i][0];
        MMDB_search_node_s node;
        int status = MMDB_read_node(mmdb, node_number, &node);
        if (MMDB_SUCCESS == status) {
            cmp_ok(node.left_record, "==", tests[i][1],
                   "left record for node %i is %i - %i bit DB",
                   node_number, tests[i][1], record_size);
            cmp_ok(node.right_record, "==", tests[i][2],
                   "left record for node %i is %i - %i bit DB",
                   node_number, tests[i][2], record_size);
        } else {
            diag("call to MMDB_read_node for node %i failed - %i bit DB",
                 node_number,
                 record_size);
        }
    }
}

void run_24_bit_record_tests(int mode, const char *mode_desc)
{
    const char *filename = "MaxMind-DB-test-mixed-24.mmdb";
    const char *path = test_database_path(filename);
    MMDB_s *mmdb = open_ok(path, mode, mode_desc);
    free((void *)path);

    const uint32_t tests[5][3] = {
        { 0,   1,   242 },
        { 80,  81,  197 },
        { 96,  97,  242 },
        { 103, 242, 104 },
        { 241, 96,  242 }
    };

    run_read_node_tests(mmdb, tests, 5, 24);

    MMDB_close(mmdb);
    free(mmdb);
}

void run_28_bit_record_tests(int mode, const char *mode_desc)
{
    const char *filename = "MaxMind-DB-test-mixed-28.mmdb";
    const char *path = test_database_path(filename);
    MMDB_s *mmdb = open_ok(path, mode, mode_desc);
    free((void *)path);

    const uint32_t tests[5][3] = {
        { 0,   1,   242 },
        { 80,  81,  197 },
        { 96,  97,  242 },
        { 103, 242, 104 },
        { 241, 96,  242 }
    };

    run_read_node_tests(mmdb, tests, 5, 28);

    MMDB_close(mmdb);
    free(mmdb);
}

void run_32_bit_record_tests(int mode, const char *mode_desc)
{
    const char *filename = "MaxMind-DB-test-mixed-32.mmdb";
    const char *path = test_database_path(filename);
    MMDB_s *mmdb = open_ok(path, mode, mode_desc);
    free((void *)path);

    const uint32_t tests[5][3] = {
        { 0,   1,   242 },
        { 80,  81,  197 },
        { 96,  97,  242 },
        { 103, 242, 104 },
        { 241, 96,  242 }
    };

    run_read_node_tests(mmdb, tests, 5, 32);

    MMDB_close(mmdb);
    free(mmdb);
}

void run_tests(int mode, const char *mode_desc)
{
    run_24_bit_record_tests(mode, mode_desc);
    run_28_bit_record_tests(mode, mode_desc);
    run_32_bit_record_tests(mode, mode_desc);
}

int main(void)
{
    plan(NO_PLAN);
    for_all_modes(&run_tests);
    done_testing();
}
