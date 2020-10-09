add_library(maxminddb
  maxminddb.c
  data-pool.c
)

if(WIN32)
  target_link_libraries(maxminddb ws2_32)
endif()

set_target_properties(maxminddb
  PROPERTIES PUBLIC_HEADER
  "../include/maxminddb_config.h;../include/maxminddb.h"
  SOVERSION ${MAXMINDDB_SOVERSION}
)

install(
  TARGETS maxminddb
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
