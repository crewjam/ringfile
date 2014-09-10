// Copyright (c) 2014 Ross Kinder. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RINGFILE_H_
#define RINGFILE_H_

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct RINGFILE;

//
// Writing example::
//
//   char * message = "Hello, World!"
//   RINGFILE * f = ringfile_open("hello.bin", "a")
//   ringfile_resize(f, 100 * 1024 * 1024);
//   ringfile_write(message, strlen(message), f)
//   ringfile_close(f);
//
// Reading example::
//
//   RINGFILE * f = ringfile_open("hello.bin", "r");
//   while (!ringfile_eof(f)) {
//     size_t size = ringfile_next_record_size(f);
//     char * buffer = malloc(size);
//     ringfile_read(buffer, size, f);
//   }
//

// Open the file for reading. Mode is one of 'r' or 'a'
RINGFILE * ringfile_open(const char * path, const char * mode);
RINGFILE * ringfile_fdopen(int fd, const char * mode);

size_t ringfile_write(const void * ptr, size_t size, RINGFILE * stream);

// Read the next record into the `size` byte buffer specified by `ptr`.
// Returns the number of bytes copied into ptr. In case the provided
// buffer is too small, the function returns -1 and no bytes are 
// copied.
size_t ringfile_read(void * ptr, size_t size, RINGFILE * stream);

// Return the size of the next record
size_t ringfile_next_record_size(RINGFILE * stream);

int ringfile_close(RINGFILE * stream);
int ringfile_eof(RINGFILE * stream);
int ringfile_error(RINGFILE * stream);
int ringfile_fileno(RINGFILE * stream);

#ifdef __cplusplus
}
#endif

#endif  // RINGFILE_H_

