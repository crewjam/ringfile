// Copyright (c) 2014 Ross Kinder. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains implementations of the functions declared in the public
// header.

#include <errno.h>
#include <ringfile.h>
#include <string.h>

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

int ringfile_write(const void * ptr, size_t size, RINGFILE * self) {
  if (!self->impl_.Write(ptr, size)) {
    errno = self->impl_.error();
    return -1;
  }
  return 0;
}

int ringfile_read(void * ptr, size_t size, RINGFILE * self) {
  if (!self->impl_.Read(ptr, size)) {
    errno = self->impl_.error();
    return -1;
  }
  return 0;
}

int ringfile_next_record_size(RINGFILE * self, size_t * size) {
  if (!self->impl_.NextRecordSize(size)) {
    errno = self->impl_.error();
    return -1;
  }
  return 0;
}

void ringfile_close(RINGFILE * self) {
  delete self;
}
