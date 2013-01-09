#include "MMDB.h"
#include <sys/stat.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>

static struct in6_addr IPNUM128_NULL = { };

static int __IN6_ADDR_IS_NULL(struct in6_addr ipnum)
{
    int i;
    for (i = 0; i < 4; i++) {
        if (ipnum.__u6_addr.__u6_addr32[i])
            return 0;
    }
#if 0
    // more portable but slow.
    for (i = 0; i < 16; i++) {
        if (v6.s6_addr[i])
            return 0;
    }
#endif
    return 1;
}

static uint32_t _get_uint32(const uint8_t * p)
{
    return (p[0] * 16777216UL + p[1] * 65536 + p[2] * 256 + p[3]);
}

static uint32_t _get_uint24(const uint8_t * p)
{
    return (p[0] * 65536UL + p[1] * 256 + p[2]);
}

static uint32_t _get_uint16(const uint8_t * p)
{
    return (p[0] * 256UL + p[1]);
}

static uint32_t _get_uintX(const uint8_t * p, int length)
{
    uint32_t r = 0;
    while (length-- > 0) {
        r <<= 8;
        r += *p++;
    }
    return r;
}

static double _get_double(const uint8_t * ptr, int length)
{
    char fmt[256];
    double d;
    sprintf(fmt, "%%%dlf", length);
    sscanf((const char *)ptr, fmt, &d);
    return (d);
}

static int _read(int fd, uint8_t * buffer, ssize_t to_read, off_t offset)
{
    while (to_read > 0) {
        ssize_t have_read = pread(fd, buffer, to_read, offset);
        if (have_read <= 0)
            return MMDB_IOERROR;
        to_read -= have_read;
        if (to_read == 0)
            break;
        offset += have_read;
        buffer += have_read;
    }
    return MMDB_SUCCESS;
}

static uint32_t _get_ptr_from(uint8_t ctrl, uint8_t * ptr, int ptr_size)
{
    uint32_t new_offset;
    switch (psize) {
    case 0:
        new_offset = (ctrl & 7) * 256 + ptr[0];
        break;
    case 1:
        new_offset = 2048 + (ctrl & 7) * 65536 + ptr[0] * 256 + ptr[1];
        break;
    case 2:
        new_offset = 2048 + 524288 + (ctrl & 7) * 16777216 + _get_uint24(ptr);
        break;
    case 3:
    default:
        new_offset = _get_uint32(ptr);
        break;
    }
    return new_offset;
}

static int _fddecode_key(MMDB_s * ipdb, int offset, MMDB_decode_key_s * ret_key)
{
    const int segments = ipdb->segments * ipdb->recbits * 2 / 8;;
    uint8_t ctrl;
    int type;
    uint8_t b[4];
    int fd = ipdb->fd;
    if (_read(fd, &ctrl, 1, segments + offset++) != MMDB_SUCCESS)
        return MMDB_IOERROR;
    type = (ctrl >> 5) & 7;
    if (type == MMDB_DTYPE_EXT) {
        if (_read(fd, &b[0], 1, segments + offset++) != MMDB_SUCCESS)
            return MMDB_IOERROR;
        type = 8 + b[0];
    }

    if (type == MMDB_DTYPE_PTR) {
        int psize = (ctrl >> 3) & 3;
        if (_read(fd, &b[0], psize + 1, segments + offset) != MMDB_SUCCESS)
            return MMDB_IOERROR;

        uint32_t new_offset = _get_ptr_from(ctrl, b, psize);

        if (_fddecode_key(ipdb, new_offset, ret_key) != MMDB_SUCCESS)
            return MMDB_IOERROR;
        ret_key->new_offset = offset + psize + 1;
        return MMDB_SUCCESS;
    }

    int size = ctrl & 31;
    switch (size) {
    case 29:
        if (_read(fd, &b[0], 1, segments + offset++) != MMDB_SUCCESS)
            return MMDB_IOERROR;
        size = 29 + b[0];
        break;
    case 30:
        if (_read(fd, &b[0], 2, segments + offset) != MMDB_SUCCESS)
            return MMDB_IOERROR;
        size = 285 + b[0] * 256 + b[1];
        offset += 2;
        break;
    case 31:
        if (_read(fd, &b[0], 3, segments + offset) != MMDB_SUCCESS)
            return MMDB_IOERROR;
        size = 65821 + _get_uint24(b);
        offset += 3;
    default:
        break;
    }

    if (size == 0) {
        ret_key->ptr = NULL;
        ret_key->size = 0;
        ret_key->new_offset = offset;
        return MMDB_SUCCESS;
    }

    ret_key->ptr = (void *)0 + segments + offset;
    ret_key->size = size;
    ret_key->new_offset = offset + size;
    return MMDB_SUCCESS;
}

#define MMDB_CHKBIT_128(bit,ptr) ((ptr)[((127UL - (bit)) >> 3)] & (1UL << (~(127UL - (bit)) & 7)))

void MMDB_free_all(MMDB_s * ipdb)
{
    if (ipdb) {
        if (ipdb->fd >= 0)
            close(ipdb->fd);
        if (ipdb->file_in_mem_ptr)
            free((void *)ipdb->file_in_mem_ptr);
        free((void *)ipdb);
    }
}

static int
_fdlookup_by_ipnum(MMDB_s * ipdb, uint32_t ipnum, MMDB_root_entry_s * result)
{
    int segments = ipdb->segments;
    off_t offset = 0;
    int byte_offset;
    int rl = ipdb->recbits * 2 / 8;
    int fd = ipdb->fd;
    uint32_t mask = 0x80000000UL;
    int depth;
    uint8_t b[4];

    if (rl == 6) {
        for (depth = 32 - 1; depth >= 0; depth--, mask >>= 1) {
            if (_read
                (fd, &b[0], 3,
                 offset * rl + ((ipnum & mask) ? 3 : 0)) != MMDB_SUCCESS)
                return MMDB_IOERROR;
            offset = _get_uint24(b);
            if (offset >= segments) {
                result->netmask = 32 - depth;
                result->entry.offset = offset - segments;
                return MMDB_SUCCESS;
            }
        }
    } else if (rl == 7) {
        for (depth = 32 - 1; depth >= 0; depth--, mask >>= 1) {
            byte_offset = offset * rl;
            if (ipnum & mask) {
                if (_read(fd, &b[0], 4, byte_offset + 3) != MMDB_SUCCESS)
                    return MMDB_IOERROR;
                offset = _get_uint32(b);
                offset &= 0xfffffff;
            } else {
                if (_read(fd, &b[0], 4, byte_offset) != MMDB_SUCCESS)
                    return MMDB_IOERROR;
                offset =
                    b[0] * 65536 + b[1] * 256 + b[2] + ((b[3] & 0xf0) << 20);
            }
            if (offset >= segments) {
                result->netmask = 32 - depth;
                result->entry.offset = offset - segments;
                return MMDB_SUCCESS;
            }
        }
    } else if (rl == 8) {
        for (depth = 32 - 1; depth >= 0; depth--, mask >>= 1) {
            if (_read
                (fd, &b[0], 4,
                 offset * rl + ((ipnum & mask) ? 4 : 0)) != MMDB_SUCCESS)
                return MMDB_IOERROR;
            offset = _get_uint32(b);
            if (offset >= segments) {
                result->netmask = 32 - depth;
                result->entry.offset = offset - segments;
                return MMDB_SUCCESS;
            }
        }
    }
    //uhhh should never happen !
    return MMDB_CORRUPTDATABASE;
}

static int
_fdlookup_by_ipnum_128(MMDB_s * ipdb, struct in6_addr ipnum,
                       MMDB_root_entry_s * result)
{
    int segments = ipdb->segments;
    int offset = 0;
    int byte_offset;
    int rl = ipdb->recbits * 2 / 8;
    int fd = ipdb->fd;
    int depth;
    uint8_t b[4];
    if (rl == 6) {

        for (depth = ipdb->depth - 1; depth >= 0; depth--) {
            byte_offset = offset * rl;
            if (MMDB_CHKBIT_128(depth, (uint8_t *) & ipnum))
                byte_offset += 3;
            if (_read(fd, &b[0], 3, byte_offset) != MMDB_SUCCESS)
                return MMDB_IOERROR;
            offset = _get_uint24(b);
            if (offset >= segments) {
                result->netmask = 128 - depth;
                result->entry.offset = offset - segments;
                return MMDB_SUCCESS;
            }
        }
    } else if (rl == 7) {
        for (depth = ipdb->depth - 1; depth >= 0; depth--) {
            byte_offset = offset * rl;
            if (MMDB_CHKBIT_128(depth, (uint8_t *) & ipnum)) {
                byte_offset += 3;
                if (_read(fd, &b[0], 4, byte_offset) != MMDB_SUCCESS)
                    return MMDB_IOERROR;
                offset = _get_uint32(b);
                offset &= 0xfffffff;
            } else {

                if (_read(fd, &b[0], 4, byte_offset) != MMDB_SUCCESS)
                    return MMDB_IOERROR;
                offset =
                    b[0] * 65536 + b[1] * 256 + b[2] + ((b[3] & 0xf0) << 20);
            }
            if (offset >= segments) {
                result->netmask = 128 - depth;
                result->entry.offset = offset - segments;
                return MMDB_SUCCESS;
            }
        }
    } else if (rl == 8) {
        for (depth = ipdb->depth - 1; depth >= 0; depth--) {
            byte_offset = offset * rl;
            if (MMDB_CHKBIT_128(depth, (uint8_t *) & ipnum))
                byte_offset += 4;
            if (_read(fd, &b[0], 4, byte_offset) != MMDB_SUCCESS)
                return MMDB_IOERROR;
            offset = _get_uint32(b);
            if (offset >= segments) {
                result->netmask = 128 - depth;
                result->entry.offset = offset - segments;
                return MMDB_SUCCESS;
            }
        }
    }
    //uhhh should never happen !
    return MMDB_CORRUPTDATABASE;
}

static int
_lookup_by_ipnum_128(MMDB_s * ipdb, struct in6_addr ipnum,
                     MMDB_root_entry_s * result)
{
    int segments = ipdb->segments;
    int offset = 0;
    int rl = ipdb->recbits * 2 / 8;
    const uint8_t *mem = ipdb->file_in_mem_ptr;
    const uint8_t *p;
    int depth;
    if (rl == 6) {

        for (depth = ipdb->depth - 1; depth >= 0; depth--) {
            p = &mem[offset * rl];
            if (MMDB_CHKBIT_128(depth, (uint8_t *) & ipnum))
                p += 3;
            offset = _get_uint24(p);
            if (offset >= segments) {
                result->netmask = 128 - depth;
                result->entry.offset = offset - segments;
                return MMDB_SUCCESS;
            }
        }
    } else if (rl == 7) {
        for (depth = ipdb->depth - 1; depth >= 0; depth--) {
            p = &mem[offset * rl];
            if (MMDB_CHKBIT_128(depth, (uint8_t *) & ipnum)) {
                p += 3;
                offset = _get_uint32(p);
                offset &= 0xfffffff;
            } else {

                offset =
                    p[0] * 65536 + p[1] * 256 + p[2] + ((p[3] & 0xf0) << 20);
            }
            if (offset >= segments) {
                result->netmask = 128 - depth;
                result->entry.offset = offset - segments;
                return MMDB_SUCCESS;
            }
        }
    } else if (rl == 8) {
        for (depth = ipdb->depth - 1; depth >= 0; depth--) {
            p = &mem[offset * rl];
            if (MMDB_CHKBIT_128(depth, (uint8_t *) & ipnum))
                p += 4;
            offset = _get_uint32(p);
            if (offset >= segments) {
                result->netmask = 128 - depth;
                result->entry.offset = offset - segments;
                return MMDB_SUCCESS;
            }
        }
    }
    //uhhh should never happen !
    return MMDB_CORRUPTDATABASE;
}

static int
_lookup_by_ipnum(MMDB_s * ipdb, uint32_t ipnum, MMDB_root_entry_s * res)
{
    int segments = ipdb->segments;
    int offset = 0;
    int rl = ipdb->recbits * 2 / 8;
    const uint8_t *mem = ipdb->file_in_mem_ptr;
    const uint8_t *p;
    uint32_t mask = 0x80000000UL;
    int depth;
    if (rl == 6) {
        for (depth = 32 - 1; depth >= 0; depth--) {
            p = &mem[offset * rl];
            if (ipnum & mask)
                p += 3;
            offset = _get_uint24(p);
            if (offset >= segments) {
                res->netmask = 32 - depth;
                res->entry.offset = offset - segments;
                return MMDB_SUCCESS;
            }
            mask >>= 1;
        }
    } else if (rl == 7) {
        for (depth = 32 - 1; depth >= 0; depth--) {
            p = &mem[offset * rl];
            if (ipnum & mask) {
                p += 3;
                offset = _get_uint32(p);
                offset &= 0xfffffff;
            } else {
                offset =
                    p[0] * 65536 + p[1] * 256 + p[2] + ((p[3] & 0xf0) << 20);
            }
            if (offset >= segments) {
                res->netmask = 32 - depth;
                res->entry.offset = offset - segments;
                return MMDB_SUCCESS;
            }
            mask >>= 1;
        }
    } else if (rl == 8) {
        for (depth = 32 - 1; depth >= 0; depth--) {
            p = &mem[offset * rl];
            if (ipnum & mask)
                p += 4;
            offset = _get_uint32(p);
            if (offset >= segments) {
                res->netmask = 32 - depth;
                res->entry.offset = offset - segments;
                return MMDB_SUCCESS;
            }
            mask >>= 1;
        }
    }
    //uhhh should never happen !
    return MMDB_CORRUPTDATABASE;
}

static void _decode_key(MMDB_s * ipdb, int offset, MMDB_decode_key_s * ret_key)
{
    //int           segments = ipdb->segments;
    const int segments = 0;
    const uint8_t *mem = ipdb->dataptr;
    uint8_t ctrl, type;
    ctrl = mem[segments + offset++];
    type = (ctrl >> 5) & 7;
    if (type == MMDB_DTYPE_EXT) {
        type = 8 + mem[segments + offset++];
    }

    if (type == MMDB_DTYPE_PTR) {
        int psize = (ctrl >> 3) & 3;
        int new_offset = _get_ptr_from(ctrl, &mem[segments + offset], psize);

        _decode_key(ipdb, new_offset, ret_key);
        ret_key->new_offset = offset + psize + 1;
        return;
    }

    int size = ctrl & 31;
    switch (size) {
    case 29:
        size = 29 + mem[segments + offset++];
        break;
    case 30:
        size = 285 + _get_uint16(&mem[segments + offset]);
        offset += 2;
        break;
    case 31:
        size = 65821 + _get_uint24(&mem[segments + offset]);
        offset += 3;
    default:
        break;
    }

    if (size == 0) {
        ret_key->ptr = (const uint8_t *)"";
        ret_key->size = 0;
        ret_key->new_offset = offset;
        return;
    }

    ret_key->ptr = &mem[segments + offset];
    ret_key->size = size;
    ret_key->new_offset = offset + size;
    return;
}

static int _init(MMDB_s * ipdb, char *fname, uint32_t flags)
{
    struct stat s;
    int fd;
    uint8_t *ptr;
    ssize_t iread;
    ssize_t size;
    off_t offset;
    ipdb->fd = fd = open(fname, O_RDONLY);
    if (fd < 0)
        return MMDB_OPENFILEERROR;
    fstat(fd, &s);
    ipdb->flags = flags;
    if ((flags & MMDB_MODE_MASK) == MMDB_MODE_MEMORY_CACHE) {
        ipdb->fd = -1;
        size = s.st_size;
        offset = 0;
    } else {
        ipdb->fd = fd;
        size = s.st_size < 2000 ? s.st_size : 2000;
        offset = s.st_size - size;
    }
    ptr = malloc(size);
    if (ptr == NULL)
        return MMDB_INVALIDDATABASE;

    iread = pread(fd, ptr, size, offset);

    const uint8_t *p = memmem(ptr, size, "\xab\xcd\xefMaxMind.com", 14);
    if (p == NULL) {
        free(ptr);
        return MMDB_INVALIDDATABASE;
    }
    p += 14;
    ipdb->file_format = p[0] * 256 + p[1];
    ipdb->recbits = p[2];
    ipdb->depth = p[3];
    ipdb->database_type = p[4] * 256 + p[5];
    ipdb->minor_database_type = p[6] * 256 + p[7];
    ipdb->segments = p[8] * 16777216 + p[9] * 65536 + p[10] * 256 + p[11];

    if ((flags & MMDB_MODE_MASK) == MMDB_MODE_MEMORY_CACHE) {
        ipdb->file_in_mem_ptr = ptr;
        ipdb->dataptr =
            ipdb->file_in_mem_ptr + ipdb->segments * ipdb->recbits * 2 / 8;

        close(fd);
    } else {
        ipdb->dataptr =
            (const uint8_t *)0 + (ipdb->segments * ipdb->recbits * 2 / 8);
        free(ptr);
    }
    return MMDB_SUCCESS;
}

MMDB_s *MMDB_open(char *fname, uint32_t flags)
{
    MMDB_s *ipdb = calloc(1, sizeof(MMDB_s));
    if (MMDB_SUCCESS != _init(ipdb, fname, flags)) {
        MMDB_free_all(ipdb);
        return NULL;
    }
    return ipdb;
}
