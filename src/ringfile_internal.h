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
  bool Resize(size_t size);
  bool Write(const void * ptr, size_t size);
  size_t Read(void * ptr, size_t size);
  size_t NextRecordSize();

  bool Close();
  bool Eof();
  int error() { return error_; }
  int size_max() const;
  int size_used() const;

  // Support for writing records piecewise without knowing exactly how big the
  // record will be when writing starts.
  bool StreamingWriteStart();
  bool StreamingWrite(const void * ptr, size_t size);
  bool StreamingWriteFinish();

 private:
  // Remove the first record in the file by advancing the start offset to the
  // next record. Returns true on success.
  bool PopRecord();

  bool WrappingWrite(const void * ptr, size_t size);
  bool WrappingWriteAtOffset(uint64_t offset, const void * data, size_t size);
  bool WrappingRead(uint64_t * offset, void * ptr, size_t size);

  int fd_;
  bool fd_is_owned_;
  int error_;
  size_t size_;
  Header * header_;
  uint64_t read_offset_;

  RecordHeader partial_write_header_;
  uint64_t partial_write_header_offset_;
};

#endif  // RINGFILE_INTERNAL_H_



