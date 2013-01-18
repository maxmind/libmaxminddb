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
static void DPRINT_KEY(MMDB_return_s * data);

static uint32_t get_uint_value(MMDB_entry_s * start, ...);
static int fdskip_hash_array(MMDB_s * mmdb, MMDB_decode_s * decode);
static void skip_hash_array(MMDB_s * mmdb, MMDB_decode_s * decode);
static int fdvget_value(MMDB_entry_s * start, MMDB_return_s * result,
                         va_list params);
static int fdcmp(MMDB_s * mmdb, MMDB_return_s const * const result, char *src_key);


int MMDB_vget_value(MMDB_entry_s * start, MMDB_return_s * result,
                    va_list params);

static struct in6_addr IPNUM128_NULL = { };

static int IN6_ADDR_IS_NULL(struct in6_addr ipnum)
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

static uint32_t get_uint32(const uint8_t * p)
{
    return (p[0] * 16777216U + p[1] * 65536 + p[2] * 256 + p[3]);
}

static int get_sint32(const uint8_t * p)
{
    return (int)(p[0] * 16777216U + p[1] * 65536 + p[2] * 256 + p[3]);
}

static uint32_t get_uint24(const uint8_t * p)
{
    return (p[0] * 65536U + p[1] * 256 + p[2]);
}

static uint32_t get_uint16(const uint8_t * p)
{
    return (p[0] * 256U + p[1]);
}

static uint32_t get_uintX(const uint8_t * p, int length)
{
    uint32_t r = 0;
    while (length-- > 0) {
        r <<= 8;
        r += *p++;
    }
    return r;
}

static double get_double(const uint8_t * ptr, int length)
{
    char fmt[256];
    double d;
    sprintf(fmt, "%%%dlf", length);
    sscanf((const char *)ptr, fmt, &d);
    return (d);
}

static int atomic_read(int fd, uint8_t * buffer, ssize_t toatomic_read, off_t offset)
{
    while (toatomic_read > 0) {
        ssize_t haveatomic_read = pread(fd, buffer, toatomic_read, offset);
        if (haveatomic_read <= 0)
            return MMDB_IOERROR;
        toatomic_read -= haveatomic_read;
        if (toatomic_read == 0)
            break;
        offset += haveatomic_read;
        buffer += haveatomic_read;
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
        new_offset = (ctrl & 7) * 16777216 + get_uint24(ptr);
#else
        new_offset = 2048 + 524288 + (ctrl & 7) * 16777216 + get_uint24(ptr);
#endif
        break;
    case 3:
    default:
        new_offset = get_uint32(ptr);
        break;
    }
    return new_offset;
}

static char *bytesdup(MMDB_return_s const *const ret)
{
    char *mem = NULL;
    if (ret->offset) {
        mem = malloc(ret->data_size + 1);
        memcpy(mem, ret->ptr, ret->data_size);
        mem[ret->data_size] = '\0';
    }
    return mem;
}

// 0 match like strcmp
int MMDB_strcmp_result(MMDB_s * mmdb, MMDB_return_s const *const result,
                       char *str)
{
    if ((mmdb->flags & MMDB_MODE_MASK) == MMDB_MODE_MEMORY_CACHE) {

        if (result->offset > 0) {
            char *str1 = bytesdup(result);
            int ret = strcmp(str1, str);
            if (str1)
                free(str1);
            return ret;
        }
        return 1;
    }

    /* MMDB_MODE_STANDARD */
    return fdcmp(mmdb, result, str);

}

static int fdcmp(MMDB_s * mmdb, MMDB_return_s const * const result, char *src_key)
{
    int src_keylen = result->data_size;
    if (result->data_size != src_keylen)
        return 1;
    uint32_t segments = mmdb->recbits * 2 / 8U * mmdb->segments;
    ssize_t offset = (ssize_t) result->ptr;
    uint8_t buff[1024];
    int len = src_keylen;
    while (len > 0) {
        int want_atomic_read = len > sizeof(buff) ? sizeof(buff) : len;
        int err = atomic_read(mmdb->fd, &buff[0], want_atomic_read, segments + offset);
        if (err == MMDB_SUCCESS) {
            if (memcmp(buff, src_key, want_atomic_read))
                return 1;       // does not match
            src_key += want_atomic_read;
            offset += want_atomic_read;
            len -= want_atomic_read;
        } else {
	  /* in case of read error */
	  return 1;
	}
    }
    return 0;
}

static int get_ext_type(int raw_ext_type)
{
#if defined BROKEN_TYPE
    return raw_ext_type;
#else
    return 8 + raw_ext_type;
#endif
}

#define FD_RET_ON_ERR(fn) do{ \
  int err = (fn);             \
  if ( err != MMDB_SUCCESS )  \
    return err;               \
  }while(0)

static int fddecode_one(MMDB_s * mmdb, uint32_t offset, MMDB_decode_s * decode)
{
    const ssize_t segments = mmdb->recbits * 2 / 8U * mmdb->segments;
    uint8_t ctrl;
    int type;
    uint8_t b[4];
    int fd = mmdb->fd;
    decode->data.offset = offset;
    FD_RET_ON_ERR(atomic_read(fd, &ctrl, 1, segments + offset++));
    type = (ctrl >> 5) & 7;
    if (type == MMDB_DTYPE_EXT) {
        FD_RET_ON_ERR(atomic_read(fd, &b[0], 1, segments + offset++));
	type = get_ext_type(b[0]);
    }

    decode->data.type = type;

    if (type == MMDB_DTYPE_PTR) {
        int psize = (ctrl >> 3) & 3;
        FD_RET_ON_ERR(atomic_read(fd, &b[0], psize + 1, segments + offset));

        decode->data.uinteger = _get_ptr_from(ctrl, b, psize);
        decode->data.used_bytes = psize + 1;
        decode->offset_to_next = offset + psize + 1;
        return MMDB_SUCCESS;
    }

    int size = ctrl & 31;
    switch (size) {
    case 29:
        FD_RET_ON_ERR(atomic_read(fd, &b[0], 1, segments + offset++));
        size = 29 + b[0];
        break;
    case 30:
        FD_RET_ON_ERR(atomic_read(fd, &b[0], 2, segments + offset));
        size = 285 + b[0] * 256 + b[1];
        offset += 2;
        break;
    case 31:
        FD_RET_ON_ERR(atomic_read(fd, &b[0], 3, segments + offset));
        size = 65821 + get_uint24(b);
        offset += 3;
    default:
        break;
    }

    if (type == MMDB_DTYPE_HASH || type == MMDB_DTYPE_ARRAY) {
        decode->data.data_size = size;
        decode->offset_to_next = offset;
        return MMDB_SUCCESS;
    }

    if (size == 0 && type != MMDB_DTYPE_UINT16 && type != MMDB_DTYPE_UINT32
        && type != MMDB_DTYPE_INT32) {
        decode->data.ptr = NULL;
        decode->data.data_size = 0;
        decode->offset_to_next = offset;
        return MMDB_SUCCESS;
    }

    if ((type == MMDB_DTYPE_UINT32) || (type == MMDB_DTYPE_UINT16)) {
        FD_RET_ON_ERR(atomic_read(fd, &b[0], size, segments + offset));
        decode->data.uinteger = get_uintX(b, size);
    } else if (type == MMDB_DTYPE_INT32) {
        FD_RET_ON_ERR(atomic_read(fd, &b[0], 4, segments + offset));
        decode->data.sinteger = get_sint32(b);
    } else if (type == MMDB_DTYPE_DOUBLE) {
        FD_RET_ON_ERR(atomic_read(fd, &b[0], size, segments + offset));
        decode->data.double_value = get_double(b, size);
    } else {
        decode->data.ptr = NULL + offset;
        decode->data.data_size = size;
    }
    decode->offset_to_next = offset + size;
    return MMDB_SUCCESS;
}

static int _fddecode_key(MMDB_s * mmdb, uint32_t offset,
                         MMDB_decode_s * ret_key)
{
    const int segments = mmdb->segments * mmdb->recbits * 2 / 8;;
    uint8_t ctrl;
    int type;
    uint8_t b[4];
    int fd = mmdb->fd;
    FD_RET_ON_ERR(atomic_read(fd, &ctrl, 1, segments + offset++));
    type = (ctrl >> 5) & 7;
    if (type == MMDB_DTYPE_EXT) {
        FD_RET_ON_ERR(atomic_read(fd, &b[0], 1, segments + offset++));
	type = get_ext_type(b[0]);
    }

    if (type == MMDB_DTYPE_PTR) {
        int psize = (ctrl >> 3) & 3;
        FD_RET_ON_ERR(atomic_read(fd, &b[0], psize + 1, segments + offset));

        uint32_t new_offset = _get_ptr_from(ctrl, b, psize);

        FD_RET_ON_ERR(_fddecode_key(mmdb, new_offset, ret_key));
        ret_key->offset_to_next = offset + psize + 1;
        return MMDB_SUCCESS;
    }

    int size = ctrl & 31;
    switch (size) {
    case 29:
        FD_RET_ON_ERR(atomic_read(fd, &b[0], 1, segments + offset++));
        size = 29 + b[0];
        break;
    case 30:
        FD_RET_ON_ERR(atomic_read(fd, &b[0], 2, segments + offset));
        size = 285 + b[0] * 256 + b[1];
        offset += 2;
        break;
    case 31:
        FD_RET_ON_ERR(atomic_read(fd, &b[0], 3, segments + offset));
        size = 65821 + get_uint24(b);
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

#if defined BROKEN_SEARCHTREE

#define RETURN_ON_END_OF_SEARCHX(offset,segments,depth,maxdepth, res)  \
            if ((offset) == 0 || (offset) >= (segments)) {    \
                (res)->netmask = (maxdepth) - (depth);                \
                (res)->entry.offset = (offset);               \
                return MMDB_SUCCESS;                          \
            }
#else
#define RETURN_ON_END_OF_SEARCHX(offset,segments,depth,maxdepth, res) \
            if ((offset) >= (segments)) {                     \
                (res)->netmask = (maxdepth) - (depth);        \
                (res)->entry.offset = (offset) - (segments);  \
                return MMDB_SUCCESS;                          \
            }
#endif

#define RETURN_ON_END_OF_SEARCH32(offset,segments,depth, res) \
	    RETURN_ON_END_OF_SEARCHX(offset,segments,depth, 32, res)

#define RETURN_ON_END_OF_SEARCH128(offset,segments,depth, res) \
	    RETURN_ON_END_OF_SEARCHX(offset,segments,depth,128, res)

int MMDB_fdlookup_by_ipnum(uint32_t ipnum, MMDB_root_entry_s * result)
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
            FD_RET_ON_ERR(atomic_read
                          (fd, &b[0], 3,
                           offset * rl + ((ipnum & mask) ? 3 : 0)));
            offset = get_uint24(b);
            RETURN_ON_END_OF_SEARCH32(offset, segments, depth, result);
        }
    } else if (rl == 7) {
        for (depth = 32 - 1; depth >= 0; depth--, mask >>= 1) {
            byte_offset = offset * rl;
            if (ipnum & mask) {
                FD_RET_ON_ERR(atomic_read(fd, &b[0], 4, byte_offset + 3));
                offset = get_uint32(b);
                offset &= 0xfffffff;
            } else {
                FD_RET_ON_ERR(atomic_read(fd, &b[0], 4, byte_offset));
                offset =
                    b[0] * 65536 + b[1] * 256 + b[2] + ((b[3] & 0xf0) << 20);
            }
            RETURN_ON_END_OF_SEARCH32(offset, segments, depth, result);
        }
    } else if (rl == 8) {
        for (depth = 32 - 1; depth >= 0; depth--, mask >>= 1) {
            FD_RET_ON_ERR(atomic_read
                          (fd, &b[0], 4,
                           offset * rl + ((ipnum & mask) ? 4 : 0)));
            offset = get_uint32(b);
            RETURN_ON_END_OF_SEARCH32(offset, segments, depth, result);
        }
    }
    //uhhh should never happen !
    return MMDB_CORRUPTDATABASE;
}

int
MMDB_fdlookup_by_ipnum_128(struct in6_addr ipnum, MMDB_root_entry_s * result)
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
            FD_RET_ON_ERR(atomic_read(fd, &b[0], 3, byte_offset));
            offset = get_uint24(b);
            RETURN_ON_END_OF_SEARCH128(offset, segments, depth, result);
        }
    } else if (rl == 7) {
        for (depth = mmdb->depth - 1; depth >= 0; depth--) {
            byte_offset = offset * rl;
            if (MMDB_CHKBIT_128(depth, (uint8_t *) & ipnum)) {
                byte_offset += 3;
                FD_RET_ON_ERR(atomic_read(fd, &b[0], 4, byte_offset));
                offset = get_uint32(b);
                offset &= 0xfffffff;
            } else {

                FD_RET_ON_ERR(atomic_read(fd, &b[0], 4, byte_offset));
                offset =
                    b[0] * 65536 + b[1] * 256 + b[2] + ((b[3] & 0xf0) << 20);
            }
            RETURN_ON_END_OF_SEARCH128(offset, segments, depth, result);
        }
    } else if (rl == 8) {
        for (depth = mmdb->depth - 1; depth >= 0; depth--) {
            byte_offset = offset * rl;
            if (MMDB_CHKBIT_128(depth, (uint8_t *) & ipnum))
                byte_offset += 4;
            FD_RET_ON_ERR(atomic_read(fd, &b[0], 4, byte_offset));
            offset = get_uint32(b);
            RETURN_ON_END_OF_SEARCH128(offset, segments, depth, result);
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
            offset = get_uint24(p);
            RETURN_ON_END_OF_SEARCH128(offset, segments, depth, result);
        }
    } else if (rl == 7) {
        for (depth = mmdb->depth - 1; depth >= 0; depth--) {
            p = &mem[offset * rl];
            if (MMDB_CHKBIT_128(depth, (uint8_t *) & ipnum)) {
                p += 3;
                offset = get_uint32(p);
                offset &= 0xfffffff;
            } else {

                offset =
                    p[0] * 65536 + p[1] * 256 + p[2] + ((p[3] & 0xf0) << 20);
            }
            RETURN_ON_END_OF_SEARCH128(offset, segments, depth, result);
        }
    } else if (rl == 8) {
        for (depth = mmdb->depth - 1; depth >= 0; depth--) {
            p = &mem[offset * rl];
            if (MMDB_CHKBIT_128(depth, (uint8_t *) & ipnum))
                p += 4;
            offset = get_uint32(p);
            RETURN_ON_END_OF_SEARCH128(offset, segments, depth, result);
        }
    }
    //uhhh should never happen !
    return MMDB_CORRUPTDATABASE;
}

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
        for (depth = 32 - 1; depth >= 0; depth--, mask >>= 1) {
            p = &mem[offset * rl];
            if (ipnum & mask)
                p += 3;
            offset = get_uint24(p);
            RETURN_ON_END_OF_SEARCH32(offset, segments, depth, res);
        }
    } else if (rl == 7) {
        for (depth = 32 - 1; depth >= 0; depth--, mask >>= 1) {
            p = &mem[offset * rl];
            if (ipnum & mask) {
                p += 3;
                offset = get_uint32(p);
                offset &= 0xfffffff;
            } else {
                offset =
                    p[0] * 65536 + p[1] * 256 + p[2] + ((p[3] & 0xf0) << 20);
            }
            RETURN_ON_END_OF_SEARCH32(offset, segments, depth, res);
        }
    } else if (rl == 8) {
        for (depth = 32 - 1; depth >= 0; depth--, mask >>= 1) {
            p = &mem[offset * rl];
            if (ipnum & mask)
                p += 4;
            offset = get_uint32(p);
            RETURN_ON_END_OF_SEARCH32(offset, segments, depth, res);
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
        size = 285 + get_uint16(&mem[offset]);
        offset += 2;
        break;
    case 31:
        size = 65821 + get_uint24(&mem[offset]);
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
        get_uint_value(&meta, KEYS("binary_format_major_version"));

    mmdb->minor_file_format =
        get_uint_value(&meta, KEYS("binary_format_minor_version"));

    // looks like the dataabase_type is the info string.
    // mmdb->database_type = get_uint_value(&meta, KEYS("database_type"));
    mmdb->recbits = get_uint_value(&meta, KEYS("record_size"));
    mmdb->segments = get_uint_value(&meta, KEYS("node_count"));

    // unfortunately we must guess the depth of the database
    mmdb->depth = get_uint_value(&meta, KEYS("ip_version")) == 4 ? 32 : 128;

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
uint32_t MMDBget_uint(MMDB_return_s const *const result)
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

static uint32_t get_uint_value(MMDB_entry_s * start, ...)
{
    MMDB_return_s result;
    va_list params;
    va_start(params, start);
    MMDB_vget_value(start, &result, params);
    va_end(params);
    return MMDBget_uint(&result);
}

static void decode_one(MMDB_s * mmdb, uint32_t offset, MMDB_decode_s * decode)
{
    const uint8_t *mem = mmdb->dataptr;
    const uint8_t *p;
    uint8_t ctrl;
    int type;
    decode->data.offset = offset;
    ctrl = mem[offset++];
    type = (ctrl >> 5) & 7;
    if (type == MMDB_DTYPE_EXT)
        type = get_ext_type(mem[offset++]);

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
        size = 285 + get_uint16(&mem[offset]);
        offset += 2;
        break;
    case 31:
        size = 65821 + get_uint24(&mem[offset]);
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
        decode->data.uinteger = get_uintX(&mem[offset], size);
    } else if (type == MMDB_DTYPE_INT32) {
        decode->data.sinteger = get_sint32(&mem[offset]);
    } else if (type == MMDB_DTYPE_DOUBLE) {
        decode->data.double_value = get_double(&mem[offset], size);
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
    if ((mmdb->flags & MMDB_MODE_MASK) == MMDB_MODE_STANDARD)
        return fdvget_value(start, result, params);
    char *src_key;              // = va_arg(params, char *);
    int src_keylen;
    while ((src_key = va_arg(params, char *))) {
        decode_one(mmdb, offset, &decode);
 donotdecode:
        src_keylen = strlen(src_key);
        switch (decode.data.type) {
        case MMDB_DTYPE_PTR:
            // we follow the pointer
            decode_one(mmdb, decode.data.uinteger, &decode);
            break;

            // learn to skip this
        case MMDB_DTYPE_ARRAY:
            skip_hash_array(mmdb, &decode);
            break;
        case MMDB_DTYPE_HASH:
            {
                int size = decode.data.data_size;
                // printf("decode hash with %d keys\n", size);
                offset = decode.offset_to_next;
                while (size--) {
                    decode_one(mmdb, offset, &key);

                    uint32_t offset_to_value = key.offset_to_next;

                    if (key.data.type == MMDB_DTYPE_PTR) {
                        // while (key.data.type == MMDB_DTYPE_PTR) {
                        decode_one(mmdb, key.data.uinteger, &key);
                        // }
                    }

                    assert(key.data.type == MMDB_DTYPE_BYTES ||
                           key.data.type == MMDB_DTYPE_UTF8_STRING);

                    if (key.data.data_size == src_keylen &&
                        !memcmp(src_key, key.data.ptr, src_keylen)) {
                        if ((src_key = va_arg(params, char *))) {
                            // DPRINT_KEY(&key.data);
                            decode_one(mmdb, offset_to_value, &decode);
                            if (decode.data.type == MMDB_DTYPE_PTR)
                                decode_one(mmdb, decode.data.uinteger,
                                            &decode);
                            //memcpy(&decode, &valudde, sizeof(MMDB_decode_s));

                            //skip_hash_array(mmdb, &value);
                            offset = decode.offset_to_next;

                            goto donotdecode;
                        }
                        // found it!
                        decode_one(mmdb, offset_to_value, &value);
                        if (value.data.type == MMDB_DTYPE_PTR)
                            decode_one(mmdb, value.data.uinteger, &value);

                        memcpy(result, &value.data, sizeof(MMDB_return_s));
                        goto end;
                    } else {
                        // we search for another key skip  this
                        decode_one(mmdb, offset_to_value, &value);
                        skip_hash_array(mmdb, &value);
                        offset = value.offset_to_next;
                    }
                }
                // not found!! do something
                //DPRINT_KEY(&key.data);
                //
                result->offset = 0;     // not found.
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

static int fdvget_value(MMDB_entry_s * start, MMDB_return_s * result,
                         va_list params)
{
    MMDB_decode_s decode, key, value;
    MMDB_s *mmdb = start->mmdb;
    uint32_t offset = start->offset;
    char *src_key;
    int src_keylen;
    int err;
    while ((src_key = va_arg(params, char *))) {

        FD_RET_ON_ERR(fddecode_one(mmdb, offset, &decode));
 donotdecode:
        src_keylen = strlen(src_key);
        switch (decode.data.type) {
        case MMDB_DTYPE_PTR:
            // we follow the pointer
            FD_RET_ON_ERR(fddecode_one(mmdb, decode.data.uinteger, &decode));
            break;

            // learn to skip this
        case MMDB_DTYPE_ARRAY:
            FD_RET_ON_ERR(fdskip_hash_array(mmdb, &decode));
            break;
        case MMDB_DTYPE_HASH:
            {
                int size = decode.data.data_size;
                //printf("decode hash with %d keys\n", size);
                offset = decode.offset_to_next;
                while (size--) {
                    FD_RET_ON_ERR(fddecode_one(mmdb, offset, &key));

                    uint32_t offset_to_value = key.offset_to_next;

                    if (key.data.type == MMDB_DTYPE_PTR) {
                        // while (key.data.type == MMDB_DTYPE_PTR) {
                        FD_RET_ON_ERR(fddecode_one
                                      (mmdb, key.data.uinteger, &key));
                        // }
                    }

                    assert(key.data.type == MMDB_DTYPE_BYTES ||
                           key.data.type == MMDB_DTYPE_UTF8_STRING);

                    if (key.data.data_size == src_keylen &&
                        !fdcmp(mmdb, &key.data, src_key)) {
                        if ((src_key = va_arg(params, char *))) {
                            //DPRINT_KEY(&key.data);
                            FD_RET_ON_ERR(fddecode_one
                                          (mmdb, offset_to_value, &decode));

                            if (decode.data.type == MMDB_DTYPE_PTR) {
                                FD_RET_ON_ERR(fddecode_one
                                              (mmdb, decode.data.uinteger,
                                               &decode));
                            }
                            //memcpy(&decode, &valudde, sizeof(MMDB_decode_s));

                            //skip_hash_array(mmdb, &value);
                            offset = decode.offset_to_next;

                            goto donotdecode;
                        }
                        // found it!
                        FD_RET_ON_ERR(fddecode_one
                                      (mmdb, offset_to_value, &value));

                        if (value.data.type == MMDB_DTYPE_PTR) {
                            FD_RET_ON_ERR(fddecode_one
                                          (mmdb, value.data.uinteger, &value));
                        }
                        memcpy(result, &value.data, sizeof(MMDB_return_s));
                        goto end;
                    } else {
                        // we search for another key skip  this
                        FD_RET_ON_ERR(fddecode_one
                                      (mmdb, offset_to_value, &value));

                        FD_RET_ON_ERR(fdskip_hash_array(mmdb, &value));
                        offset = value.offset_to_next;
                    }
                }
                // not found!! do something
                //DPRINT_KEY(&key.data);
                //
                result->offset = 0;     // not found.
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

static int fdskip_hash_array(MMDB_s * mmdb, MMDB_decode_s * decode)
{
    int err;
    if (decode->data.type == MMDB_DTYPE_HASH) {
        int size = decode->data.data_size;
        while (size--) {
            FD_RET_ON_ERR(fddecode_one(mmdb, decode->offset_to_next, decode)); // key
            FD_RET_ON_ERR(fddecode_one(mmdb, decode->offset_to_next, decode)); // value
            FD_RET_ON_ERR(fdskip_hash_array(mmdb, decode));
        }

    } else if (decode->data.type == MMDB_DTYPE_ARRAY) {
        int size = decode->data.data_size;
        while (size--) {
            FD_RET_ON_ERR(fddecode_one(mmdb, decode->offset_to_next, decode)); // value
            FD_RET_ON_ERR(fdskip_hash_array(mmdb, decode));
        }
    }
    return MMDB_SUCCESS;
}

static void skip_hash_array(MMDB_s * mmdb, MMDB_decode_s * decode)
{
    if (decode->data.type == MMDB_DTYPE_HASH) {
        int size = decode->data.data_size;
        while (size--) {
            decode_one(mmdb, decode->offset_to_next, decode);  // key
            decode_one(mmdb, decode->offset_to_next, decode);  // value
            skip_hash_array(mmdb, decode);
        }

    } else if (decode->data.type == MMDB_DTYPE_ARRAY) {
        int size = decode->data.data_size;
        while (size--) {
            decode_one(mmdb, decode->offset_to_next, decode);  // value
            skip_hash_array(mmdb, decode);
        }
    }
}

static void DPRINT_KEY(MMDB_return_s * data)
{
    char str[256];
    int len = data->data_size > 255 ? 255 : data->data_size;
    memcpy(str, data->ptr, len);
    str[len] = '\0';
    fprintf(stderr, "%s\n", str);
}
