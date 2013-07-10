/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */

#include <Python.h>
#include <MMDB.h>
#include <netdb.h>

staticforward PyTypeObject MMDB_MMDBType;
static PyObject *mkobj_r(MMDB_s * mmdb, MMDB_decode_all_s ** current);



/* Exception object for python */
static PyObject *PyMMDBError;

typedef struct {
    PyObject_HEAD               /* no semicolon */
    MMDB_s * mmdb;
} MMDB_MMDBObject;


// Create a new Python MMDB object
static PyObject *MMDB_new_Py(PyObject * self, PyObject * args)
{
    MMDB_MMDBObject *obj;
    char *filename;
    int flags;

    if (!PyArg_ParseTuple(args, "si", &filename, &flags)) {
        return NULL;
    }

    obj = PyObject_New(MMDB_MMDBObject, &MMDB_MMDBType);
    if (!obj)
        return NULL;

    obj->mmdb = MMDB_open(filename, flags);
    if (!obj->mmdb) {
        PyErr_SetString(PyMMDBError, "Can't create obj->mmdb object");
        Py_DECREF(obj);
        return NULL;
    }
    return (PyObject *) obj;
}

// Destroy the MMDB object
static void MMDB_MMDB_dealloc(PyObject * self)
{
    MMDB_MMDBObject *obj = (MMDB_MMDBObject *) self;
    MMDB_close(obj->mmdb);
    PyObject_Del(self);
}

// This function creates the Py object for us 
static PyObject *mkobj(MMDB_s * mmdb, MMDB_decode_all_s ** current)
{
    MMDB_decode_all_s *tmp = *current;
    PyObject *py = mkobj_r(mmdb, current);
    *current = tmp;
    return py;
}

// Return the version of the CAPI
static PyObject *MMDB_lib_version_Py(PyObject * self, PyObject * args)
{
    return Py_BuildValue("s", MMDB_lib_version());
}

// This function is used to do the lookup
static PyObject *MMDB_lookup_Py(PyObject * self, PyObject * args)
{
    char *name;
    struct in6_addr ip;
    int status;

    MMDB_MMDBObject *obj = (MMDB_MMDBObject *) self;
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return NULL;
    }

    status = MMDB_lookupaddressX(name, AF_INET6, AI_V4MAPPED, &ip);
    if (status == 0) {
        MMDB_root_entry_s root = {.entry.mmdb = obj->mmdb };
        status = MMDB_lookup_by_ipnum_128(ip, &root);
        if (status == MMDB_SUCCESS && root.entry.offset > 0) {
            MMDB_decode_all_s *decode_all;
            if (MMDB_get_tree(&root.entry, &decode_all) == MMDB_SUCCESS) {
                PyObject *retval = mkobj(obj->mmdb, &decode_all);
                MMDB_free_decode_all(decode_all);
                return retval;
            }
        }
    }
    Py_RETURN_NONE;
}



// minor helper fuction to create a python string from the database
static PyObject *build_PyString_FromStringAndSize(MMDB_s * mmdb, void *ptr,
                                                  int size)
{
    const int BSIZE = 256;
    char buffer[BSIZE];
    int fd = mmdb->fd;
    if (fd < 0)
        return PyString_FromStringAndSize(ptr, size);

    void *bptr = size < BSIZE ? buffer : malloc(size);
    assert(bptr);
    uint32_t segments = mmdb->full_record_size_bytes * mmdb->node_count;
    MMDB_pread(fd, bptr, size, segments + (uintptr_t) ptr);
    PyObject *py = PyString_FromStringAndSize(bptr, size);
    if (size >= BSIZE)
        free(bptr);
    return py;
}

// minor helper fuction to create a python string from the database
static PyObject *build_PyUnicode_DecodeUTF8(MMDB_s * mmdb, void *ptr, int size)
{
    const int BSIZE = 256;
    char buffer[BSIZE];
    int fd = mmdb->fd;
    if (fd < 0)
        return PyUnicode_DecodeUTF8(ptr, size, NULL);

    void *bptr = size < BSIZE ? buffer : malloc(size);
    assert(bptr);
    uint32_t segments = mmdb->full_record_size_bytes * mmdb->node_count;
    MMDB_pread(fd, bptr, size, segments + (uintptr_t) ptr);
    PyObject *py = PyUnicode_DecodeUTF8(bptr, size, NULL);
    if (size >= BSIZE)
        free(bptr);
    return py;
}

// iterated over our datastructure and create python from it
static PyObject *mkobj_r(MMDB_s * mmdb, MMDB_decode_all_s ** current)
{
    PyObject *sv = NULL;
    switch ((*current)->decode.data.type) {
    case MMDB_DTYPE_MAP:
        {
            PyObject *hv = PyDict_New();
            int size = (*current)->decode.data.data_size;
            for (*current = (*current)->next; size; size--) {
                PyObject *key, *val;
                int key_size = (*current)->decode.data.data_size;
                void *key_ptr = size ? (void *)(*current)->decode.data.ptr : "";
                *current = (*current)->next;
                val = mkobj_r(mmdb, current);
                key =
                    build_PyString_FromStringAndSize(mmdb, key_ptr,
                                                     key_size);
                PyDict_SetItem(hv, key, val);
                Py_DECREF(val);
                Py_DECREF(key);
            }
            return hv;
        }
        break;
    case MMDB_DTYPE_ARRAY:
        {
            int size = (*current)->decode.data.data_size;
            PyObject *av = PyList_New(0);
            for (*current = (*current)->next; size; size--) {
                PyObject *val = mkobj_r(mmdb, current);
                PyList_Append(av, val);
                Py_DECREF(val);
            }
            return av;
        }
        break;
    case MMDB_DTYPE_UTF8_STRING:
        {
            int size = (*current)->decode.data.data_size;
            void *ptr = size ? (void*)(*current)->decode.data.ptr : "";
            sv = build_PyUnicode_DecodeUTF8(mmdb, ptr, size);
        }
        break;
    case MMDB_DTYPE_BYTES:
        {
            int size = (*current)->decode.data.data_size;
            sv = build_PyString_FromStringAndSize(mmdb,
                                                  (void *)(*current)->
                                                  decode.data.ptr, size);
        }
        break;
    case MMDB_DTYPE_IEEE754_FLOAT:
        sv = Py_BuildValue("d", (*current)->decode.data.float_value);
        break;
    case MMDB_DTYPE_IEEE754_DOUBLE:
        sv = Py_BuildValue("d", (*current)->decode.data.double_value);
        break;
    case MMDB_DTYPE_UINT32:
        sv = Py_BuildValue("I", (*current)->decode.data.uinteger);
        break;
    case MMDB_DTYPE_UINT64:
        sv = build_PyString_FromStringAndSize(mmdb,
                                              (void *)(*current)->decode.
                                              data.c8, 8);
        break;
    case MMDB_DTYPE_UINT128:
        sv = build_PyString_FromStringAndSize(mmdb,
                                              (void *)(*current)->decode.
                                              data.c16, 16);
        break;
    case MMDB_DTYPE_BOOLEAN:
    case MMDB_DTYPE_UINT16:
    case MMDB_DTYPE_INT32:
        sv = PyInt_FromLong((*current)->decode.data.sinteger);
        break;
    default:
        assert(0);
    }

    if (*current)
        *current = (*current)->next;

    return sv;
}

static PyMethodDef MMDB_Object_methods[] = {
    {"lookup", MMDB_lookup_Py, 1, "Lookup entry by ipaddr"},
    {NULL, NULL, 0, NULL}
};

static PyObject *MMDB_GetAttr(PyObject * self, char *attrname)
{
    return Py_FindMethod(MMDB_Object_methods, self, attrname);
}

static PyTypeObject MMDB_MMDBType = {
    PyObject_HEAD_INIT(NULL)
        0,
    "MMDB",
    sizeof(MMDB_MMDBObject),
    0,
    MMDB_MMDB_dealloc,          /*tp_dealloc */
    0,                          /*tp_print */
    (getattrfunc) MMDB_GetAttr, /*tp_getattr */
    0,                          /*tp_setattr */
    0,                          /*tp_compare */
    0,                          /*tp_repr */
    0,                          /*tp_as_number */
    0,                          /*tp_as_sequence */
    0,                          /*tp_as_mapping */
    0,                          /*tp_hash */
};

static PyMethodDef MMDB_Class_methods[] = {
    {"new", MMDB_new_Py, 1,
     "MMDB Constructor with database filename argument"},
    {"lib_version", MMDB_lib_version_Py, 1, "Returns the CAPI version"},
    {NULL, NULL, 0, NULL}
};

DL_EXPORT(void) initMMDB(void)
{
    PyObject *m, *d, *tmp;
    MMDB_MMDBType.ob_type = &PyType_Type;

    m = Py_InitModule("MMDB", MMDB_Class_methods);
    d = PyModule_GetDict(m);

    PyMMDBError = PyErr_NewException("py_mmdb.error", NULL, NULL);
    PyDict_SetItemString(d, "error", PyMMDBError);

    tmp = PyInt_FromLong(MMDB_MODE_STANDARD);
    PyDict_SetItemString(d, "MMDB_MODE_STANDARD", tmp);
    Py_DECREF(tmp);

    tmp = PyInt_FromLong(MMDB_MODE_MEMORY_CACHE);
    PyDict_SetItemString(d, "MMDB_MODE_MEMORY_CACHE", tmp);
    Py_DECREF(tmp);
}
