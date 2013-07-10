#!/usr/bin/python

import MMDB
print MMDB.lib_version();
mmdb =MMDB.new("/usr/local/share/GeoIP2/GeoIP2-City.mmdb",MMDB.MMDB_MODE_MEMORY_CACHE )

print mmdb.lookup("24.24.24.24")

