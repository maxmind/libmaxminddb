cmake_minimum_required (VERSION 3.9)
project(maxminddb
  LANGUAGES C
  VERSION 1.4.3
)

add_definitions(-DPACKAGE_VERSION="${PROJECT_VERSION}")

include(CheckTypeSize)
check_type_size("unsigned __int128" UINT128)
check_type_size("unsigned int __attribute__((mode(TI)))" UINT128_USING_MODE)
if(HAVE_UINT128)
  set(MMDB_UINT128_USING_MODE 0)
  set(MMDB_UINT128_IS_BYTE_ARRAY 0)
elseif(HAVE_UINT128_USING_MODE)
  set(MMDB_UINT128_USING_MODE 1)
  set(MMDB_UINT128_IS_BYTE_ARRAY 0)
else()
  set(MMDB_UINT128_USING_MODE 0)
  set(MMDB_UINT128_IS_BYTE_ARRAY 1)
endif()

set(MMDB_UINT128_USING_MODE, HAVE__int128)

include (TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if(NOT IS_BIG_ENDIAN)
  add_definitions(-DMMDB_LITTLE_ENDIAN=1)
endif()

set(CMAKE_SHARED_LIBRARY_PREFIX lib)
set(CMAKE_STATIC_LIBRARY_PREFIX lib)

configure_file(include/maxminddb_config.h.cmake.in include/maxminddb_config.h)

include_directories(. src include)

add_subdirectory(src)
add_subdirectory(bin)

enable_testing()
add_subdirectory(t)

