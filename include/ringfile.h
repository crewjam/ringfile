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
struct RINGFILE * ringfile_create(const char * path, size_t size);

// Open the file for reading. Mode is one of 'r' (read) or 'a' (append).
struct RINGFILE * ringfile_open(const char * path, const char * mode);

// TODO(ross): not yet implemented
// RINGFILE * ringfile_fdopen(int fd, const char * mode);

// Write a record to the file. 
// Returns 0 on success. On failure, returns -1 and sets errno.
int ringfile_write(const void * ptr, size_t size, struct RINGFILE * stream);

// Read the next record into the `size` byte buffer specified by `ptr`.
// Returns 0 on sucess. On failure, returns -1 and sets errno.
int ringfile_read(void * ptr, size_t size, struct RINGFILE * stream);

// Fill in `size` with the size of the next record in the file. Returns 0 on 
// success or -1 on failure.
int ringfile_next_record_size(RINGFILE * self, size_t * size);

// Close the the specified stream and free resources associated with it. Do not
// reference `stream` again after this call.
void ringfile_close(struct RINGFILE * stream);

#ifdef __cplusplus
}
#endif

#endif  // RINGFILE_H_
