cmake_minimum_required (VERSION 3.9)
project(maxminddb
  LANGUAGES C
  VERSION 1.4.3
)

add_definitions(-DPACKAGE_VERSION="${PROJECT_VERSION}")

include (TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if(NOT IS_BIG_ENDIAN)
  add_definitions(-DMMDB_LITTLE_ENDIAN=1)
endif()

set(CMAKE_SHARED_LIBRARY_PREFIX lib)
set(CMAKE_STATIC_LIBRARY_PREFIX lib)

configure_file(include/maxminddb_config.h.in include/maxminddb_config.h)

include_directories(. src include)

add_library(maxminddb
  src/maxminddb.c
  src/data-pool.c
)

if(WIN32)
  target_link_libraries(maxminddb ws2_32)
endif()

set_target_properties(maxminddb
  PROPERTIES PUBLIC_HEADER
  "include/maxminddb_config.h;include/maxminddb.h"
)

install(
  TARGETS maxminddb
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

add_executable(mmdblookup
  bin/mmdblookup.c
)

target_link_libraries(mmdblookup maxminddb pthread)

install(
  TARGETS mmdblookup
  RUNTIME DESTINATION bin
)

add_library(tap
  t/libtap/tap.c
)

enable_testing()

file(GLOB tests "t/*_t.c")
foreach(file ${tests})
  string(REGEX REPLACE ".*/t/([a-z0-9_]*)\.c" "\\1" test_name ${file})

  add_executable(${test_name}
    ${file}
    t/maxminddb_test_helper.c
  )

  target_link_libraries(${test_name} maxminddb tap pthread)

  add_test(${test_name} ${test_name})
endforeach()
