// Copyright (c) 2014 Ross Kinder. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef RINGFILE_INTERNAL_H_
#define RINGFILE_INTERNAL_H_

#include <ringfile.h>
#include <stdint.h>
#include <sys/types.h>

#include <string>

#if !defined(__cplusplus)
#error C++ only
#endif

#pragma pack(push, 1)
struct Header {
  uint32_t magic;
  uint32_t flags;
  uint64_t start_offset;
  uint64_t end_offset;
};

struct RecordHeader {
  uint32_t size;
};
#pragma pack(pop)

class Ringfile {
 public:
  Ringfile();
  ~Ringfile();

  enum Mode { kRead, kAppend };
  static const uint32_t kMagic = 'GNIR';

  bool Create(const std::string & path, size_t size);
  bool Open(const std::string & path, Mode mode);
  bool Write(const void * ptr, size_t size);
  bool Read(void * ptr, size_t size);
  bool NextRecordSize(size_t * size);
  bool EndOfFile();

  bool Close();
  
  int error() { return error_; }
  size_t bytes_max() const;
  size_t bytes_used() const;
  size_t bytes_available() const;

  // Support for writing records piecewise without knowing exactly how big the
  // record will be when writing starts.
  bool StreamingWriteStart();
  bool StreamingWrite(const void * ptr, size_t size);
  bool StreamingWriteFinish();
  
  // Functions that support reading partial records
  size_t StreamingReadStart();
  size_t StreamingRead(void * ptr, size_t size);
  bool StreamingReadFinish();
  
 private:
  // Set the file pointer to `offset`.
  // Note: whenever we refer to a file offset it is relative to beginning of the
  // data area of the file, not relative to the beginning of the file. Also, 
  // all offsets are interpreted modulo the data size (bytes_max()).
  bool SeekToOffset(uint64_t offset);
  
  // Remove the first record in the file by advancing the start offset to the
  // next record. Returns true on success.
  bool PopRecord();

  bool WrappingWrite(uint64_t offset, const void * data, size_t size);
  bool WrappingRead(uint64_t offset, void * ptr, size_t size);

  int fd_;
  bool fd_is_owned_;
  int error_;
  size_t size_;
  Header * header_;
  uint64_t read_offset_;

  RecordHeader partial_write_header_;
  uint64_t partial_write_header_offset_;
  
  RecordHeader partial_read_header_;
  uint64_t partial_read_offset_;
};

#endif  // RINGFILE_INTERNAL_H_



