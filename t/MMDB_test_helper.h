#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <math.h>
#include <netdb.h>
#include <string.h>
#include "MMDB.h"
#include "libtap/tap.h"

#ifndef MMDB_TEST_HELPER_C
#define MMDB_TEST_HELPER_C (1)

#define MAX_DESCRIPTION_LENGTH 500

void for_all_modes(void (*tests) (int mode, const char *description));

const char *test_database_path(const char *filename);

MMDB_s *open_ok(const char *db_file, int mode, const char *mode_desc);

MMDB_lookup_result_s *lookup_ok(MMDB_s *mmdb, const char *ip,
                                const char *file, const char *mode_desc);

MMDB_return_s data_ok(MMDB_lookup_result_s *result, int data_type,
                      const char *description, ...);

#endif
