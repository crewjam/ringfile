// Copyright (c) 2014 Ross Kinder. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef PYTHON_MODULE_H_
#define PYTHON_MODULE_H_

#include <Python.h>
#include "ringfile_internal.h"

typedef struct {
  PyObject_HEAD
  Ringfile * impl;
} RingfileObject;

static void Ringfile_dealloc(RingfileObject * self);
static PyObject * Ringfile_new(PyTypeObject * type, PyObject * args,
    PyObject * kwargs);
static int Ringfile_init(RingfileObject * self, PyObject * args,
    PyObject * kwargs);
static PyObject * Ringfile_create(PyObject * cls, PyObject * args,
  PyObject * kwargs);
static PyObject * Ringfile_write(RingfileObject *self, PyObject *args,
  PyObject *kwargs);
static PyObject * Ringfile_read(RingfileObject *self);
static PyObject * Ringfile_iter(RingfileObject *self);
static PyObject * Ringfile_iternext(RingfileObject *self);
static PyObject * Ringfile_close(RingfileObject *self);

#endif  // PYTHON_MODULE_H_
