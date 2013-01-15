#include "MMDB.h"
#include <sys/stat.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#define KEYS(...) __VA_ARGS__, NULL

// prototypes
//
void _DPRINT_KEY(MMDB_return_s * data);

uint32_t _get_uint_value(MMDB_entry_s * start, ...);
void _skip_hash_array(MMDB_s * mmdb, MMDB_decode_s * decode);

int MMDB_vget_value(MMDB_entry_s * start, MMDB_return_s * result,
                    va_list params);

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
    return (p[0] * 16777216U + p[1] * 65536 + p[2] * 256 + p[3]);
}

static int _get_sint32(const uint8_t * p)
{
    return (int)(p[0] * 16777216U + p[1] * 65536 + p[2] * 256 + p[3]);
}

static uint32_t _get_uint24(const uint8_t * p)
{
    return (p[0] * 65536U + p[1] * 256 + p[2]);
}

static uint32_t _get_uint16(const uint8_t * p)
{
    return (p[0] * 256U + p[1]);
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

static uint32_t _get_ptr_from(uint8_t ctrl, uint8_t const *const ptr,
                              int ptr_size)
{
    uint32_t new_offset;
    switch (ptr_size) {
    case 0:
        new_offset = (ctrl & 7) * 256 + ptr[0];
        break;
    case 1:
#if defined BROKEN_PTR
        new_offset = (ctrl & 7) * 65536 + ptr[0] * 256 + ptr[1];
#else
        new_offset = 2048 + (ctrl & 7) * 65536 + ptr[0] * 256 + ptr[1];
#endif
        break;
    case 2:
#if defined BROKEN_PTR
        new_offset = (ctrl & 7) * 16777216 + _get_uint24(ptr);
#else
        new_offset = 2048 + 524288 + (ctrl & 7) * 16777216 + _get_uint24(ptr);
#endif
        break;
    case 3:
    default:
        new_offset = _get_uint32(ptr);
        break;
    }
    return new_offset;
}

static int _fddecode_key(MMDB_s * mmdb, uint32_t offset, MMDB_decode_s * ret_key)
{
    const int segments = mmdb->segments * mmdb->recbits * 2 / 8;;
    uint8_t ctrl;
    int type;
    uint8_t b[4];
    int fd = mmdb->fd;
    if (_read(fd, &ctrl, 1, segments + offset++) != MMDB_SUCCESS)
        return MMDB_IOERROR;
    type = (ctrl >> 5) & 7;
    if (type == MMDB_DTYPE_EXT) {
        if (_read(fd, &b[0], 1, segments + offset++) != MMDB_SUCCESS)
            return MMDB_IOERROR;
#if defined BROKEN_TYPE
        type = b[0];
#else
        type = 8 + b[0];
#endif
    }

    if (type == MMDB_DTYPE_PTR) {
        int psize = (ctrl >> 3) & 3;
        if (_read(fd, &b[0], psize + 1, segments + offset) != MMDB_SUCCESS)
            return MMDB_IOERROR;

        uint32_t new_offset = _get_ptr_from(ctrl, b, psize);

        if (_fddecode_key(mmdb, new_offset, ret_key) != MMDB_SUCCESS)
            return MMDB_IOERROR;
        ret_key->offset_to_next = offset + psize + 1;
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
        ret_key->data.ptr = NULL;
        ret_key->data.data_size = 0;
        ret_key->offset_to_next = offset;
        return MMDB_SUCCESS;
    }

    ret_key->data.ptr = (void *)0 + segments + offset;
    ret_key->data.data_size = size;
    ret_key->offset_to_next = offset + size;
    return MMDB_SUCCESS;
}

#define MMDB_CHKBIT_128(bit,ptr) ((ptr)[((127U - (bit)) >> 3)] & (1U << (~(127U - (bit)) & 7)))

void MMDB_free_all(MMDB_s * mmdb)
{
    if (mmdb) {
        if (mmdb->fd >= 0)
            close(mmdb->fd);
        if (mmdb->file_in_mem_ptr)
            free((void *)mmdb->file_in_mem_ptr);
        free((void *)mmdb);
    }
}

static int _fdlookup_by_ipnum(uint32_t ipnum, MMDB_root_entry_s * result)
{
    MMDB_s *mmdb = result->entry.mmdb;
    int segments = mmdb->segments;
    off_t offset = 0;
    int byte_offset;
    int rl = mmdb->recbits * 2 / 8;
    int fd = mmdb->fd;
    uint32_t mask = 0x80000000U;
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
_fdlookup_by_ipnum_128(struct in6_addr ipnum, MMDB_root_entry_s * result)
{
    MMDB_s *mmdb = result->entry.mmdb;
    int segments = mmdb->segments;
    uint32_t offset = 0;
    int byte_offset;
    int rl = mmdb->recbits * 2 / 8;
    int fd = mmdb->fd;
    int depth;
    uint8_t b[4];
    if (rl == 6) {

        for (depth = mmdb->depth - 1; depth >= 0; depth--) {
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
        for (depth = mmdb->depth - 1; depth >= 0; depth--) {
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
        for (depth = mmdb->depth - 1; depth >= 0; depth--) {
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

int MMDB_lookup_by_ipnum_128(struct in6_addr ipnum, MMDB_root_entry_s * result)
{
    MMDB_s *mmdb = result->entry.mmdb;
    int segments = mmdb->segments;
    uint32_t offset = 0;
    int rl = mmdb->recbits * 2 / 8;
    const uint8_t *mem = mmdb->file_in_mem_ptr;
    const uint8_t *p;
    int depth;
    if (rl == 6) {

        for (depth = mmdb->depth - 1; depth >= 0; depth--) {
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
        for (depth = mmdb->depth - 1; depth >= 0; depth--) {
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
        for (depth = mmdb->depth - 1; depth >= 0; depth--) {
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

#if defined BROKEN_SEARCHTREE

#define RETURN_ON_END_OF_SEARCH32(offset,segments,depth,res)  \
            if ((offset) == 0 || (offset) >= (segments)) {    \
                (res)->netmask = 32 - (depth);                \
                (res)->entry.offset = (offset);               \
                return MMDB_SUCCESS;                          \
            }

#else
#define RETURN_ON_END_OF_SEARCH32(offset,segments,depth, res) \
            if ((offset) >= (segments)) {                     \
                (res)->netmask = 32 - (depth);                \
                (res)->entry.offset = (offset) - (segments);  \
                return MMDB_SUCCESS;                          \
            }
#endif

int MMDB_lookup_by_ipnum(uint32_t ipnum, MMDB_root_entry_s * res)
{
    MMDB_s *mmdb = res->entry.mmdb;
    int segments = mmdb->segments;
    uint32_t offset = 0;
    int rl = mmdb->recbits * 2 / 8;
    const uint8_t *mem = mmdb->file_in_mem_ptr;
    const uint8_t *p;
    uint32_t mask = 0x80000000U;
    int depth;
    if (rl == 6) {
        for (depth = 32 - 1; depth >= 0; depth--) {
            p = &mem[offset * rl];
            if (ipnum & mask)
                p += 3;
            offset = _get_uint24(p);
            RETURN_ON_END_OF_SEARCH32(offset, segments, depth, res);
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
            RETURN_ON_END_OF_SEARCH32(offset, segments, depth, res);
            mask >>= 1;
        }
    } else if (rl == 8) {
        for (depth = 32 - 1; depth >= 0; depth--) {
            p = &mem[offset * rl];
            if (ipnum & mask)
                p += 4;
            offset = _get_uint32(p);
            RETURN_ON_END_OF_SEARCH32(offset, segments, depth, res);
            mask >>= 1;
        }
    }
    //uhhh should never happen !
    return MMDB_CORRUPTDATABASE;
}

static void _decode_key(MMDB_s * mmdb, uint32_t offset, MMDB_decode_s * ret_key)
{
    const uint8_t *mem = mmdb->dataptr;
    uint8_t ctrl, type;
    ctrl = mem[offset++];
    type = (ctrl >> 5) & 7;
    if (type == MMDB_DTYPE_EXT) {
#if defined BROKEN_TYPE
        type = mem[offset++];
#else
        type = 8 + mem[offset++];
#endif

    }

    if (type == MMDB_DTYPE_PTR) {
        int psize = (ctrl >> 3) & 3;
        int new_offset = _get_ptr_from(ctrl, &mem[offset], psize);

        _decode_key(mmdb, new_offset, ret_key);
        ret_key->offset_to_next = offset + psize + 1;
        return;
    }

    int size = ctrl & 31;
    switch (size) {
    case 29:
        size = 29 + mem[offset++];
        break;
    case 30:
        size = 285 + _get_uint16(&mem[offset]);
        offset += 2;
        break;
    case 31:
        size = 65821 + _get_uint24(&mem[offset]);
        offset += 3;
    default:
        break;
    }

    if (size == 0) {
        ret_key->data.ptr = (const uint8_t *)"";
        ret_key->data.data_size = 0;
        ret_key->offset_to_next = offset;
        return;
    }

    ret_key->data.ptr = &mem[offset];
    ret_key->data.data_size = size;
    ret_key->offset_to_next = offset + size;
    return;
}

static int _init(MMDB_s * mmdb, char *fname, uint32_t flags)
{
    struct stat s;
    int fd;
    uint8_t *ptr;
    ssize_t iread;
    ssize_t size;
    off_t offset;
    mmdb->fd = fd = open(fname, O_RDONLY);
    if (fd < 0)
        return MMDB_OPENFILEERROR;
    fstat(fd, &s);
    mmdb->flags = flags;
    if ((flags & MMDB_MODE_MASK) == MMDB_MODE_MEMORY_CACHE) {
        mmdb->fd = -1;
        size = s.st_size;
        offset = 0;
    } else {
        mmdb->fd = fd;
        size = s.st_size < 2000 ? s.st_size : 2000;
        offset = s.st_size - size;
    }
    ptr = malloc(size);
    if (ptr == NULL)
        return MMDB_INVALIDDATABASE;

    iread = pread(fd, ptr, size, offset);

    const uint8_t *metadata = memmem(ptr, size, "\xab\xcd\xefMaxMind.com", 14);
    if (metadata == NULL) {
        free(ptr);
        return MMDB_INVALIDDATABASE;
    }

    MMDB_s fakedb = {.dataptr = metadata + 14 };
    MMDB_entry_s meta = {.mmdb = &fakedb };
    MMDB_return_s result;

    // we can't fail with ioerror's here. It is a memory operation
    mmdb->major_file_format =
        _get_uint_value(&meta, KEYS("binary_format_major_version"));

    mmdb->minor_file_format =
        _get_uint_value(&meta, KEYS("binary_format_minor_version"));

    // looks like the dataabase_type is the info string.
    // mmdb->database_type = _get_uint_value(&meta, KEYS("database_type"));
    mmdb->recbits = _get_uint_value(&meta, KEYS("record_size"));
    mmdb->segments = _get_uint_value(&meta, KEYS("node_count"));

    // unfortunately we must guess the depth of the database
    mmdb->depth = _get_uint_value(&meta, KEYS("ip_version")) == 4 ? 32 : 128;

    if ((flags & MMDB_MODE_MASK) == MMDB_MODE_MEMORY_CACHE) {
        mmdb->file_in_mem_ptr = ptr;
        mmdb->dataptr =
            mmdb->file_in_mem_ptr + mmdb->segments * mmdb->recbits * 2 / 8;

        close(fd);
    } else {
        mmdb->dataptr =
            (const uint8_t *)0 + (mmdb->segments * mmdb->recbits * 2 / 8);
        free(ptr);
    }
    return MMDB_SUCCESS;
}

MMDB_s *MMDB_open(char *fname, uint32_t flags)
{
    MMDB_s *mmdb = calloc(1, sizeof(MMDB_s));
    if (MMDB_SUCCESS != _init(mmdb, fname, flags)) {
        MMDB_free_all(mmdb);
        return NULL;
    }
    return mmdb;
}

/* return the result of any uint type with 32 bit's or less as uint32 */
uint32_t MMDB_get_uint(MMDB_return_s const *const result)
{
    return result->uinteger;
}

int MMDB_get_value(MMDB_entry_s * start, MMDB_return_s * result, ...)
{
    va_list keys;
    va_start(keys, result);
    int ioerror = MMDB_vget_value(start, result, keys);
    va_end(keys);
    return ioerror;
}

uint32_t _get_uint_value(MMDB_entry_s * start, ...)
{
    MMDB_return_s result;
    va_list params;
    va_start(params, start);
    MMDB_vget_value(start, &result, params);
    va_end(params);
    return MMDB_get_uint(&result);
}

void _decode_one(MMDB_s * mmdb, uint32_t offset, MMDB_decode_s * decode)
{
    const uint8_t *mem = mmdb->dataptr;
    const uint8_t *p;
    uint8_t ctrl;
    int type;
    decode->data.offset = offset;
    ctrl = mem[offset++];
    type = (ctrl >> 5) & 7;
    if (type == MMDB_DTYPE_EXT) {
#if defined BROKEN_TYPE
        type = mem[offset++];
#else
        type = 8 + mem[offset++];
#endif

    }

    decode->data.type = type;

    if (type == MMDB_DTYPE_PTR) {
        int psize = (ctrl >> 3) & 3;
        decode->data.uinteger = _get_ptr_from(ctrl, &mem[offset], psize);
        decode->data.used_bytes = psize + 1;
        decode->offset_to_next = offset + psize + 1;
        return;
    }

    int size = ctrl & 31;
    switch (size) {
    case 29:
        size = 29 + mem[offset++];
        break;
    case 30:
        size = 285 + _get_uint16(&mem[offset]);
        offset += 2;
        break;
    case 31:
        size = 65821 + _get_uint24(&mem[offset]);
        offset += 3;
    default:
        break;
    }

    if (type == MMDB_DTYPE_HASH || type == MMDB_DTYPE_ARRAY) {
        decode->data.data_size = size;
        decode->offset_to_next = offset;
        return;
    }

    if (size == 0 && type != MMDB_DTYPE_UINT16 && type != MMDB_DTYPE_UINT32
        && type != MMDB_DTYPE_INT32) {
        decode->data.ptr = NULL;
        decode->data.data_size = 0;
        decode->offset_to_next = offset;
        return;
    }

    if ((type == MMDB_DTYPE_UINT32) || (type == MMDB_DTYPE_UINT16)) {
        decode->data.uinteger = _get_uintX(&mem[offset], size);
    } else if (type == MMDB_DTYPE_INT32) {
        decode->data.sinteger = _get_sint32(&mem[offset]);
    } else if (type == MMDB_DTYPE_DOUBLE) {
        decode->data.double_value = _get_double(&mem[offset], size);
    } else {
        decode->data.ptr = &mem[offset];
        decode->data.data_size = size;
    }
    decode->offset_to_next = offset + size;

    return;
}

int MMDB_vget_value(MMDB_entry_s * start, MMDB_return_s * result,
                    va_list params)
{
    MMDB_decode_s decode, key, value;
    MMDB_s *mmdb = start->mmdb;
    uint32_t offset = start->offset;
    char *src_key = va_arg(params, char *);
    while (1) {
        int src_keylen = strlen(src_key);
        _decode_one(mmdb, offset, &decode);
        switch (decode.data.type) {
        case MMDB_DTYPE_PTR:
            // we follow the pointer
            _decode_one(mmdb, decode.data.uinteger, &decode);
            break;

            // learn to skip this
        case MMDB_DTYPE_ARRAY:
            _skip_hash_array(mmdb, &decode);
            break;
        case MMDB_DTYPE_HASH:
            {
                int size = decode.data.data_size;
                offset = decode.offset_to_next;
                while (size--) {
                    _decode_one(mmdb, offset, &key);

                    uint32_t offset_to_value = key.offset_to_next;

                    if (key.data.type == MMDB_DTYPE_PTR) {
                        while (key.data.type == MMDB_DTYPE_PTR) {
                            _decode_one(mmdb, key.data.uinteger, &key);
                        }
                    }

                    assert(key.data.type == MMDB_DTYPE_BYTES ||
                           key.data.type == MMDB_DTYPE_UTF8_STRING);

                    // _DPRINT_KEY(&key.data);

                    if (key.data.data_size == src_keylen &&
                        !memcmp(src_key, key.data.ptr, src_keylen)) {
                        _decode_one(mmdb, offset_to_value, &value);
                        if ((src_key = va_arg(params, char *))) {

                            _skip_hash_array(mmdb, &value);
                            offset = value.offset_to_next;
                            continue;
                        }
                        // found it!
                        memcpy(result, &value.data, sizeof(MMDB_return_s));
                        goto end;
                    } else {
                        // we search for another key skip  this
                        _decode_one(mmdb, offset_to_value, &value);
                        _skip_hash_array(mmdb, &value);
                        offset = value.offset_to_next;
                    }
                }
                // not found!! do something
                //_DPRINT_KEY(&key.data);
                goto end;
            }
        default:
            break;
        }
    }
 end:
    va_end(params);
    return MMDB_SUCCESS;
}

void _skip_hash_array(MMDB_s * mmdb, MMDB_decode_s * decode)
{
    if (decode->data.type == MMDB_DTYPE_HASH) {
        int size = decode->data.data_size;
        while (size--) {
            _decode_one(mmdb, decode->offset_to_next, decode);  // key
            _decode_one(mmdb, decode->offset_to_next, decode);  // value
            _skip_hash_array(mmdb, decode);
        }

    } else if (decode->data.type == MMDB_DTYPE_ARRAY) {
        int size = decode->data.data_size;
        while (size--) {
            _decode_one(mmdb, decode->offset_to_next, decode);  // value
            _skip_hash_array(mmdb, decode);
        }
    }
}

void _DPRINT_KEY(MMDB_return_s * data)
{
    char str[256];
    int len = data->data_size > 255 ? 255 : data->data_size;
    memcpy(str, data->ptr, len);
    str[len] = '\0';
    fprintf(stderr, "%s\n", str);
}
