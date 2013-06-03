#include "MMDB.h"
#include "tap.h"
#include <string.h>
#if HAVE_CONFIG_H
# include <config.h>
#endif

int main(void)
{

    float f = 2.57;             // 40247AE1
    double d = 2.57;            // 40048F5C28F5C28F

#if defined ( __LITTLE_ENDIAN__ )
    uint8_t *binfloat = (void *)"\xe1\x7a\x24\x40";
    uint8_t *bindouble = (void *)"\x8f\xc2\xf5\x28\x5c\x8f\x04\x40";
#else
    uint8_t *binfloat = (void *)"\x40\x24\x7a\xe1";
    uint8_t *bindouble = (void *)"\x40\x04\x8F\x5C\x28\xF5\xC2\x8F";
#endif

    ok(sizeof(float) == 4, "(sizeof ( float ) == 4");
    ok(sizeof(double) == 8, "(sizeof ( double ) == 8");

    ok(memcmp(&f, binfloat, 4) == 0, "binary float looks ok");
    ok(memcmp(&d, bindouble, 8) == 0, "binary double looks ok");

    done_testing();
}
