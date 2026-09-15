find_package(cmocka CONFIG QUIET)
if(NOT cmocka_FOUND)
  message(STATUS "cmocka not found, building it from source")
  include(FetchContent)
  # These only apply to the cmocka subdirectory.
  set(BUILD_SHARED_LIBS OFF)
  set(WITH_EXAMPLES OFF)
  set(UNIT_TESTING OFF)
  set(CMOCKA_URL https://cmocka.org/files/2.0/cmocka-2.0.1.tar.xz)
  set(CMOCKA_SHA256 3f3533382ba29ab3abf5c4f4b27b79d165f0df51ea587de749b11b68b4019180)
  # EXCLUDE_FROM_ALL keeps cmocka out of the install tree.
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.28)
    FetchContent_Declare(cmocka
      URL ${CMOCKA_URL}
      URL_HASH SHA256=${CMOCKA_SHA256}
      EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(cmocka)
  else()
    FetchContent_Declare(cmocka
      URL ${CMOCKA_URL}
      URL_HASH SHA256=${CMOCKA_SHA256}
    )
    FetchContent_GetProperties(cmocka)
    if(NOT cmocka_POPULATED)
      FetchContent_Populate(cmocka)
      add_subdirectory(${cmocka_SOURCE_DIR} ${cmocka_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()
  endif()
endif()

# test programs
set(TEST_TARGET_NAMES
  bad_pointers_t
  bad_search_tree_t
  basic_lookup_t
  data_entry_list_t
  data-pool-t
  data_types_t
  double_close_t
  dump_t
  gai_error_t
  get_value_pointer_bug_t
  get_value_t
  ipv4_start_cache_t
  ipv6_lookup_in_ipv4_t
  metadata_marker_t
  metadata_pointers_t
  metadata_t
  no_map_get_value_t
  overflow_bounds_t
  pointer_dos_t
  read_node_t
  version_t
)

if(UNIX)  # or if (NOT WIN32)
  list(APPEND TEST_TARGET_NAMES
    bad_databases_t
    bad_data_size_t
    bad_epoch_t
    bad_indent_t
    empty_container_metadata_t
    invalid_sockaddr_t
    max_depth_t
    threads_t
  )
  find_package(Threads)
endif()

foreach(TEST_TARGET_NAME ${TEST_TARGET_NAMES})
  add_executable(${TEST_TARGET_NAME} ${TEST_TARGET_NAME}.c maxminddb_test_helper.c)
  target_include_directories(${TEST_TARGET_NAME} PRIVATE ../src)
  target_link_libraries(${TEST_TARGET_NAME} maxminddb cmocka::cmocka)
  target_compile_definitions(${TEST_TARGET_NAME} PRIVATE PACKAGE_VERSION="${PROJECT_VERSION}")

  if(UNIX)
    target_link_libraries(${TEST_TARGET_NAME} m)
  endif()

  if (UNIX)
    target_link_libraries(${TEST_TARGET_NAME} ${CMAKE_THREAD_LIBS_INIT})
  endif()

  add_test( NAME ${TEST_TARGET_NAME} COMMAND ${TEST_TARGET_NAME} WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/t)
endforeach()

if(BUILD_FUZZING)
  add_executable(fuzz_mmdb fuzz_mmdb.c)
  target_include_directories(fuzz_mmdb PRIVATE ../src)
  target_link_libraries(fuzz_mmdb maxminddb $ENV{LIB_FUZZING_ENGINE})
endif()
