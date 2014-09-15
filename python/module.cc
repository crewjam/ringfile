// Copyright (c) 2014 Ross Kinder. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "module.h"

PyDoc_STRVAR(class_doc,
"A Ringfile object represents a fixed size, disk backed buffer that can be \
used to write records in a circular fashion. When the buffer is full, \
adding new records causes old records to be removed.\n\
\n\
Ringfile(path, [mode])\n\
\n\
*path* is the path to an existing file. Use the :meth:`Create` method to\n\
create new files. *mode* is one of the constants `MODE_READ`` or\n\
`MODE_APPEND`, by default `MODE_READ`.\n\
\n\
Example::\n\
\n\
  ringfile = Ringfile.Create('/var/log/foo', 4096)\n\
  ringfile.write('Hello, World!')\n\
  ringfile.write('Goodbye, World!')\n\
  ringfile.close()\n\
\n\
  ringfile = Ringfile('/var/log/foo')\n\
  assert ringfile.read() == 'Hello, World!'\n\
  assert ringfile.read() == 'Goodbye, World!'\n\
  assert ringfile.read() is None\n\
\n\
");

PyDoc_STRVAR(create_doc,
"create(path, size)\n\
\n\
Create a new ringfile an return an initialized, writeable Ringfile object. \
If the file exists or cannot be created raise IOError.");

PyDoc_STRVAR(write_doc,
"write(record)\n\
\n\
Append a record to the ringfile. Raise IOError if the write fails.");

PyDoc_STRVAR(read_doc,
"read()\n\
\n\
Read and return a record from the ringfile. Return None if there are no more \
records in the file. Raise IOError if the read fails.");

PyDoc_STRVAR(close_doc,
"close()\n\
\n\
Explicitly close the handle to the ringfile.");


static PyMethodDef Ringfile_methods[] = {
  {"create", (PyCFunction)Ringfile_create,
    METH_CLASS|METH_VARARGS|METH_KEYWORDS,
    create_doc},
  {"write", (PyCFunction)Ringfile_write, METH_VARARGS|METH_KEYWORDS,
    write_doc},
  {"read", (PyCFunction)Ringfile_read, METH_NOARGS,
    read_doc},
  {"close", (PyCFunction)Ringfile_close, METH_NOARGS,
    close_doc},
  {NULL}  //  Sentinel
};

static PyTypeObject RingfileType = {
  PyObject_HEAD_INIT(NULL)
  0,                         // ob_size
  "ringfile.Ringfile",       // tp_name
  sizeof(RingfileObject),    // tp_basicsize
  0,                         // tp_itemsize
  (destructor)Ringfile_dealloc,  // tp_dealloc
  0,                         // tp_print
  0,                         // tp_getattr
  0,                         // tp_setattr
  0,                         // tp_compare
  0,                         // tp_repr
  0,                         // tp_as_number
  0,                         // tp_as_sequence
  0,                         // tp_as_mapping
  0,                         // tp_hash
  0,                         // tp_call
  0,                         // tp_str
  0,                         // tp_getattro
  0,                         // tp_setattro
  0,                         // tp_as_buffer
  Py_TPFLAGS_DEFAULT,        // tp_flags
  class_doc,                 // tp_doc
  0,                         // tp_traverse
  0,                         // tp_clear
  0,                         // tp_richcompare
  0,                         // tp_weaklistoffset
  0,                         // tp_iter
  0,                         // tp_iternext
  Ringfile_methods,          // tp_methods
  0,                         // tp_members
  0,                         // tp_getset
  0,                         // tp_base
  0,                         // tp_dict
  0,                         // tp_descr_get
  0,                         // tp_descr_set
  0,                         // tp_dictoffset
  (initproc)Ringfile_init,   // tp_init
  0,                         // tp_alloc
  (newfunc)Ringfile_new,     // tp_new
};

static PyMethodDef module_methods[] = {
    {NULL}  //  Sentinel
};

PyMODINIT_FUNC initringfile() {
  RingfileType.tp_new = PyType_GenericNew;
  if (PyType_Ready(&RingfileType) < 0) {
    return;
  }

  PyObject * module = Py_InitModule3("ringfile", module_methods,
    "Ringfile module");
  if (module == NULL) {
    return;
  }

  Py_INCREF(&RingfileType);
  PyModule_AddObject(module, "Ringfile",
    reinterpret_cast<PyObject *>(&RingfileType));
  PyModule_AddIntConstant(module, "MODE_READ", Ringfile::kRead);
  PyModule_AddIntConstant(module, "MODE_APPEND", Ringfile::kAppend);
}

static void Ringfile_dealloc(RingfileObject * self) {
  if (self->impl) {
    delete self->impl;
    self->impl = NULL;
  }
  self->ob_type->tp_free(reinterpret_cast<PyObject *>(self));
}

static PyObject * Ringfile_new(PyTypeObject * type, PyObject * args,
    PyObject * kwargs) {
  RingfileObject * self = reinterpret_cast<RingfileObject *>(
    type->tp_alloc(type, 0));
  if (self == NULL) {
    return NULL;
  }
  self->impl = NULL;
  return reinterpret_cast<PyObject *>(self);
}

static int Ringfile_init(RingfileObject * self, PyObject * args,
    PyObject * kwargs) {
  static char * kwlist[] = {"path", "mode", NULL};
  char * path;
  Ringfile::Mode mode = Ringfile::kRead;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|i", kwlist,
      &path, &mode)) {
    return -1;
  }

  self->impl = new Ringfile();

  if (!self->impl->Open(path, mode)) {
    PyObject * error = PyInt_FromLong(self->impl->error());
    PyErr_SetObject(PyExc_IOError, error);
    Py_DECREF(error);
    return -1;
  }

  return 0;
}

static PyObject * Ringfile_create(PyObject * cls, PyObject * args,
    PyObject * kwargs) {
  static char * kwlist[] = {"path", "size", NULL};
  char * path;
  int size = -1;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "si", kwlist, &path, &size)) {
    return NULL;
  }

  RingfileObject * self = PyObject_New(RingfileObject, &RingfileType);
  self->impl = new Ringfile();

  if (!self->impl->Create(path, size)) {
    PyObject * error = PyInt_FromLong(self->impl->error());
    PyErr_SetObject(PyExc_IOError, error);
    Py_DECREF(error);

    Py_DECREF(self);
    return NULL;
  }

  return reinterpret_cast<PyObject *>(self);
}

static PyObject * Ringfile_write(RingfileObject * self, PyObject * args,
    PyObject * kwargs) {
  static char * kwlist[] = {"buffer", NULL};
  PyObject * buffer = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "S", kwlist, &buffer)) {
    return NULL;
  }
  if (!self->impl->Write(PyString_AsString(buffer), PyString_Size(buffer))) {
    PyObject * error = PyInt_FromLong(self->impl->error());
    PyErr_SetObject(PyExc_IOError, error);
    Py_DECREF(error);
    return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject * Ringfile_read(RingfileObject *self) {
  if (self->impl->EndOfFile()) {
    Py_RETURN_NONE;
  }
  
  size_t record_size;
  if (!self->impl->NextRecordSize(&record_size)) {
    PyObject * error = PyInt_FromLong(self->impl->error());
    PyErr_SetObject(PyExc_IOError, error);
    Py_DECREF(error);
    return NULL;
  }

  PyObject * buffer = PyString_FromStringAndSize(NULL, record_size);
  if (buffer == NULL) {
    return NULL;
  }

  if (_PyString_Resize(&buffer, record_size) == -1) {
    // "If the reallocation fails, the original string object at *string is
    // deallocated".
    return NULL;
  }


  if (!self->impl->Read(PyString_AS_STRING(buffer), record_size)) {
    Py_DECREF(buffer);

    PyObject * error = PyInt_FromLong(self->impl->error());
    PyErr_SetObject(PyExc_IOError, error);
    Py_DECREF(error);

    return NULL;
  }

  return buffer;
}

static PyObject * Ringfile_close(RingfileObject *self) {
  self->impl->Close();
  Py_RETURN_NONE;
}

