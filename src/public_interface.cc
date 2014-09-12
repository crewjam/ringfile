// Copyright (c) 2014 Ross Kinder. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains implementations of the functions declared in the public
// header.

#include <errno.h>
#include <ringfile.h>

#include "ringfile_internal.h"

struct RINGFILE {
  Ringfile impl_;
};

RINGFILE * ringfile_create(const char * path, size_t size) {
  RINGFILE * self = new RINGFILE();
  if (!self->impl_.Create(path, size)) {
    errno = self->impl_.error();
    delete self;
    return NULL;
  }
  return self;
}

RINGFILE * ringfile_open(const char * path, const char * mode_str) {
  Ringfile::Mode mode;
  if (strcmp(mode_str, "r") == 0) {
    mode = Ringfile::kRead;
  } else if (strcmp(mode_str, "a") == 0) {
    mode = Ringfile::kAppend;
  } else {
    return NULL;
  }

  RINGFILE * self = new RINGFILE();
  if (!self->impl_.Open(path, mode)) {
    errno = self->impl_.error();
    delete self;
    return NULL;
  }
  return self;
}

size_t ringfile_write(const void * ptr, size_t size, RINGFILE * self) {
  if (!self->impl_.Write(ptr, size)) {
    errno = self->impl_.error();
    return -1;
  }
  return size;
}

size_t ringfile_read(void * ptr, size_t size, RINGFILE * self) {
  return self->impl_.Read(ptr, size);
}

size_t ringfile_next_record_size(RINGFILE * self) {
  return self->impl_.NextRecordSize();
}

int ringfile_close(RINGFILE * self) {
  self->impl_.Close();  // redundant with the delete below
  delete self;
}
