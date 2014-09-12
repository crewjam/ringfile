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
//   RINGFILE * f = ringfile_create("hello.bin", 100 * 1024 * 1024);
//   ringfile_write(message, strlen(message), f)
//   ringfile_close(f);
//
// Reading example::
//
//   RINGFILE * f = ringfile_open("hello.bin", "r");
//   while (1) {
//     size_t size = ringfile_next_record_size(f);
//     if (size == -1) {
//       break;
//     }
//     char * buffer = malloc(size);
//     ringfile_read(buffer, size, f);
//   }
//

// Create a new ringfile. `path` must not exist.
RINGFILE * ringfile_create(const char * path, size_t size);

// Open the file for reading. Mode is one of 'r' (read) or 'a' (append).
RINGFILE * ringfile_open(const char * path, const char * mode);

// TODO(ross): not yet implemented
// RINGFILE * ringfile_fdopen(int fd, const char * mode);

size_t ringfile_write(const void * ptr, size_t size, RINGFILE * stream);

// Read the next record into the `size` byte buffer specified by `ptr`.
// Returns the number of bytes copied into ptr. In case the provided
// buffer is too small, the function returns -1 and no bytes are
// copied.
size_t ringfile_read(void * ptr, size_t size, RINGFILE * stream);

// Return the size of the next record
size_t ringfile_next_record_size(RINGFILE * stream);

// Close the the specified stream and free resources associated with it. Do not
// reference `stream` again after this call.
int ringfile_close(RINGFILE * stream);

#ifdef __cplusplus
}
#endif

#endif  // RINGFILE_H_
