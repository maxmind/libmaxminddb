add_library(tap
  libtap/tap.c
)


if(NOT WIN32)
  add_executable(bad_databases_t
    bad_databases_t.c
    maxminddb_test_helper.c
  )
  target_include_directories(bad_databases_t PRIVATE ../src)
  target_link_libraries(bad_databases_t maxminddb tap)
  add_test(bad_databases_t bad_databases_t)
endif()


add_executable(bad_pointers_t
  bad_pointers_t.c
  maxminddb_test_helper.c
)
target_include_directories(bad_pointers_t PRIVATE ../src)
target_link_libraries(bad_pointers_t maxminddb tap)
add_test(bad_pointers_t bad_pointers_t)


add_executable(basic_lookup_t
  basic_lookup_t.c
  maxminddb_test_helper.c
)
target_include_directories(basic_lookup_t PRIVATE ../src)
target_link_libraries(basic_lookup_t maxminddb tap)
add_test(basic_lookup_t basic_lookup_t)


add_executable(data_entry_list_t
  data_entry_list_t.c
  maxminddb_test_helper.c
)
target_include_directories(data_entry_list_t PRIVATE ../src)
target_link_libraries(data_entry_list_t maxminddb tap)
add_test(data_entry_list_t data_entry_list_t)


add_executable(data-pool-t
  data-pool-t.c
  maxminddb_test_helper.c
)
target_link_libraries(data-pool-t maxminddb tap)
target_include_directories(data-pool-t PRIVATE ../src)
if(NOT WIN32)
  target_link_libraries(data-pool-t m)
endif()
add_test(data-pool-t data-pool-t)


add_executable(data_types_t
  data_types_t.c
  maxminddb_test_helper.c
)
target_include_directories(data_types_t PRIVATE ../src)
target_link_libraries(data_types_t maxminddb tap)
add_test(data_types_t data_types_t)


add_executable(dump_t
  dump_t.c
  maxminddb_test_helper.c
)
target_include_directories(dump_t PRIVATE ../src)
target_link_libraries(dump_t maxminddb tap)
add_test(dump_t dump_t)


add_executable(get_value_pointer_bug_t
  get_value_pointer_bug_t.c
  maxminddb_test_helper.c
)
target_include_directories(get_value_pointer_bug_t PRIVATE ../src)
target_link_libraries(get_value_pointer_bug_t maxminddb tap)
add_test(get_value_pointer_bug_t get_value_pointer_bug_t)


add_executable(get_value_t
  get_value_t.c
  maxminddb_test_helper.c
)
target_include_directories(get_value_t PRIVATE ../src)
target_link_libraries(get_value_t maxminddb tap)
add_test(get_value_t get_value_t)


add_executable(ipv4_start_cache_t
  ipv4_start_cache_t.c
  maxminddb_test_helper.c
)
target_include_directories(ipv4_start_cache_t PRIVATE ../src)
target_link_libraries(ipv4_start_cache_t maxminddb tap)
add_test(ipv4_start_cache_t ipv4_start_cache_t)


add_executable(ipv6_lookup_in_ipv4_t
  ipv6_lookup_in_ipv4_t.c
  maxminddb_test_helper.c
)
target_include_directories(ipv6_lookup_in_ipv4_t PRIVATE ../src)
target_link_libraries(ipv6_lookup_in_ipv4_t maxminddb tap)
add_test(ipv6_lookup_in_ipv4_t ipv6_lookup_in_ipv4_t)


add_executable(metadata_pointers_t
  metadata_pointers_t.c
  maxminddb_test_helper.c
)
target_include_directories(metadata_pointers_t PRIVATE ../src)
target_link_libraries(metadata_pointers_t maxminddb tap)
add_test(metadata_pointers_t metadata_pointers_t)


add_executable(metadata_t
  metadata_t.c
  maxminddb_test_helper.c
)
target_include_directories(metadata_t PRIVATE ../src)
target_link_libraries(metadata_t maxminddb tap)
add_test(metadata_t metadata_t)


add_executable(no_map_get_value_t
  no_map_get_value_t.c
  maxminddb_test_helper.c
)
target_include_directories(no_map_get_value_t PRIVATE ../src)
target_link_libraries(no_map_get_value_t maxminddb tap)
add_test(no_map_get_value_t no_map_get_value_t)


add_executable(read_node_t
  read_node_t.c
  maxminddb_test_helper.c
)
target_include_directories(read_node_t PRIVATE ../src)
target_link_libraries(read_node_t maxminddb tap)
add_test(read_node_t read_node_t)


if(NOT WIN32)
  add_executable(threads_t
    threads_t.c
    maxminddb_test_helper.c
  )
  target_include_directories(threads_t PRIVATE ../src)
  target_link_libraries(threads_t maxminddb tap pthread)
  add_test(threads_t threads_t)
endif()


add_executable(version_t
  version_t.c
  maxminddb_test_helper.c
)
target_include_directories(version_t PRIVATE ../src)
target_link_libraries(version_t maxminddb tap)
add_test(version_t version_t)
