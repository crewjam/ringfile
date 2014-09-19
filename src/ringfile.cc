// Copyright (c) 2014 Ross Kinder. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "ringfile_internal.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include "varint.h"

Ringfile::Ringfile()
  : fd_(-1),
    fd_is_owned_(false),
    header_(NULL),
    streaming_read_offset_(0),
    streaming_write_offset_(0) {
}

Ringfile::~Ringfile() {
  Close();
}

bool Ringfile::Create(const std::string & path, size_t size) {
  mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  fd_ = open(path.c_str(), O_RDWR|O_CREAT|O_EXCL, mode);
  if (fd_ == -1) {
    error_ = errno;
    return false;
  }
  fd_is_owned_ = true;

  if (ftruncate(fd_, size) == -1) {
    error_ = errno;
    return false;
  }
  size_ = size;

  header_ = reinterpret_cast<Header *>(mmap(0, sizeof(*header_),
    mode == kRead ? 0 : PROT_WRITE, MAP_SHARED, fd_, 0));
  if (!header_) {
    error_ = errno;
    return false;
  }

  header_->magic = kMagic;
  header_->flags = 0;
  header_->start_offset = 0;
  header_->end_offset = 0;

  return true;
}

bool Ringfile::Open(const std::string & path, Ringfile::Mode mode) {
  if (mode == kRead) {
    fd_ = open(path.c_str(), O_RDONLY);
  } else if (mode == kAppend) {
    fd_ = open(path.c_str(), O_RDWR);
  }

  if (fd_ == -1) {
    error_ = errno;
    return false;
  }
  fd_is_owned_ = true;

  {
    struct stat stat_buffer;
    if (fstat(fd_, &stat_buffer) == -1) {
      error_ = errno;
      return false;
    }
    size_ = stat_buffer.st_size;
  }

  header_ = reinterpret_cast<Header *>(mmap(0, sizeof(*header_),
    mode == kRead ? PROT_READ : PROT_READ|PROT_WRITE, MAP_SHARED, fd_, 0));
  if (!header_) {
    error_ = errno;
    Close();
    return false;
  }

  if (header_->magic != kMagic) {
    Close();
    error_ = EINVAL;  // invalid magic number
    return false;
  }

  read_offset_ = header_->start_offset;
  return true;
}

bool Ringfile::SeekToOffset(uint64_t offset) {
  assert(offset < bytes_max()); // DO NOT COMMIT
  offset %= bytes_max();
  if (lseek(fd_, sizeof(Header) + offset, SEEK_SET) == -1) {
    error_ = errno;
    return false;
  }
  return true;
}

bool Ringfile::WrappingRead(uint64_t offset, void * ptr, size_t size) {
  offset %= bytes_max();
  uint64_t end_offset = (offset + size) % bytes_max();

  uint64_t end_bytes;
  uint64_t start_bytes;
  if (offset < end_offset) {
    // simple case: the whole write is at the end
    end_bytes = size;
    start_bytes = 0;
  } else {
    // complex case: part of the write is at the end and part at the start
    end_bytes = bytes_max() - offset;
    start_bytes = size - end_bytes;
  }

  if (end_bytes) {
    if (!SeekToOffset(offset)) {
      return false;
    }
    if (read(fd_, ptr, end_bytes) != end_bytes) {
      return false;
    }
  }

  if (start_bytes) {
    if (!SeekToOffset(0)) {
      return false;
    }
    if (read(fd_, reinterpret_cast<char *>(ptr) + end_bytes,
        start_bytes) != start_bytes) {
      return false;
    }
  }
  return true;
}

bool Ringfile::WrappingWrite(uint64_t offset, const void * ptr, size_t size) {
  offset %= bytes_max();
  uint64_t end_offset = (offset + size) % bytes_max();

  uint64_t end_bytes;
  uint64_t start_bytes;
  if (offset < end_offset) {
    // simple case: the whole write is at the end
    end_bytes = size;
    start_bytes = 0;
  } else {
    // complex case: part of the write is at the end and part at the start
    end_bytes = bytes_max() - offset;
    start_bytes = size - end_bytes;
  }

  if (end_bytes) {
    if (!SeekToOffset(offset)) {
      return false;
    }
    if (write(fd_, ptr, end_bytes) != end_bytes) {
      return false;
    }
  }

  if (start_bytes) {
    if (!SeekToOffset(0)) {
      return false;
    }
    if (write(fd_, reinterpret_cast<const char *>(ptr) + end_bytes,
        start_bytes) != start_bytes) {
      return false;
    }
  }

  return true;
}

bool Ringfile::PopRecord() {
  if (header_->start_offset == header_->end_offset) {
    // Empty
    return false;
  }

  // Read the first record header
  uint8_t header_buffer[Varint::kMaxSize];
  if (!WrappingRead(header_->start_offset, header_buffer, Varint::kMaxSize)) {
    return false;
  }
  Varint size_varint;
  int header_size = size_varint.Read(&header_buffer);

  // Advance the start pointer to the end of the record.
  header_->start_offset += header_size + size_varint.value();
  header_->start_offset %= bytes_max();

  // Reset an empty list (optional)
  //if (header_->start_offset == header_->end_offset) {
  //  header_->start_offset = 0;
  //  header_->end_offset = 0;
  //}

  return true;
}

bool Ringfile::EndOfFile() {
  return read_offset_ == header_->end_offset;
}

bool Ringfile::NextRecordSize(size_t * size) {
  if (!header_ || fd_ == -1) {
    return false;
  }
  if (read_offset_ == header_->end_offset) {
    return false;
  }

  uint8_t header_buffer[Varint::kMaxSize];
  if (!WrappingRead(read_offset_, header_buffer, Varint::kMaxSize)) {
    return false;
  }

  Varint size_varint;
  int header_size = size_varint.Read(header_buffer);
  *size = size_varint.value();
  return true;
}

bool Ringfile::Read(void * buffer, size_t buffer_size) {
  if (!header_ || fd_ == -1) {
    return false;
  }

  uint8_t header_buffer[Varint::kMaxSize];
  if (!WrappingRead(read_offset_, header_buffer, Varint::kMaxSize)) {
    return false;
  }

  Varint size_varint;
  int header_size = size_varint.Read(header_buffer);

  if (size_varint.value() > buffer_size) {
    return false;
  }

  if (!WrappingRead(read_offset_ + header_size, buffer, size_varint.value())) {
    return false;
  }
  read_offset_ += header_size + size_varint.value();
  read_offset_ %= bytes_max();
  return true;
}

bool Ringfile::Write(const void * ptr, size_t size) {
  // Build the header
  uint8_t header_buffer[Varint::kMaxSize];
  Varint size_varint(size);
  int header_size = size_varint.ByteSize();
  size_varint.Write(&header_buffer);

  // Refuse a record that is too big for the buffer
  if (bytes_max() < (header_size + size + 1)) {
    return false;
  }

  // Pop records until there is enough space available
  while (bytes_available() < (header_size + size)) {
    size_t bytes_available_start = bytes_available();
    if (!PopRecord()) {
      return false;
    }
    assert(bytes_available() > bytes_available_start);
  }


  if (!WrappingWrite(header_->end_offset, header_buffer, header_size)) {
    return false;
  }
  if (!WrappingWrite(header_->end_offset + header_size, ptr, size)) {
    return false;
  }

  header_->end_offset += header_size + size;
  header_->end_offset %= bytes_max();

  return true;
}

bool Ringfile::Close() {
  if (header_) {
    munmap(header_, sizeof(*header_));
    header_ = NULL;
  }

  if (fd_ != -1 && fd_is_owned_) {
    close(fd_);
    fd_ = -1;
  }
  return true;
}

size_t Ringfile::bytes_max() const {
  return size_ - sizeof(Header);
}

size_t Ringfile::bytes_used() const {
  if (header_->start_offset <= header_->end_offset) {
    return header_->end_offset - header_->start_offset;
  } else {
    return bytes_max() - (header_->start_offset - header_->end_offset);
  }
}

size_t Ringfile::bytes_available() const {
  return bytes_max() - bytes_used();
}

bool Ringfile::StreamingWriteStart(size_t size) {
  assert(streaming_write_offset_ == 0);

  // Build the header
  uint8_t header_buffer[Varint::kMaxSize];
  Varint size_varint(size);
  int header_size = size_varint.ByteSize();
  size_varint.Write(&header_buffer);

  // Refuse a record that is too big for the buffer
  if (bytes_max() < (header_size + size + 1)) {
    return false;
  }

  // Pop records until there is enough space available
  while (bytes_available() < (header_size + size)) {
    size_t bytes_available_start = bytes_available();
    if (!PopRecord()) {
      return false;
    }
    assert(bytes_available() > bytes_available_start);
  }

  if (!WrappingWrite(header_->end_offset, header_buffer, header_size)) {
    return false;
  }

  streaming_write_offset_ = header_->end_offset + header_size;
  streaming_write_bytes_remaining_ = size;

  return true;
}

bool Ringfile::StreamingWrite(const void * ptr, size_t size) {
  assert(size <= streaming_write_bytes_remaining_);

  if (!WrappingWrite(streaming_write_offset_, ptr, size)) {
    return false;
  }
  streaming_write_offset_ += size;
  streaming_write_bytes_remaining_ -= size;
  return true;
}

bool Ringfile::StreamingWriteFinish() {
  assert(streaming_write_bytes_remaining_ == 0);
  header_->end_offset = streaming_write_offset_;
  header_->end_offset %= bytes_max();
  streaming_write_offset_ = 0;
  return true;
}

size_t Ringfile::StreamingReadStart() {
  assert(header_ != NULL);
  assert(fd_ != -1);
  assert(streaming_read_offset_ == 0);

  if (read_offset_ == header_->end_offset) {
    return -1;
  }

  uint8_t header_buffer[Varint::kMaxSize];
  if (!WrappingRead(read_offset_, header_buffer, Varint::kMaxSize)) {
    return -1;
  }

  Varint size_varint;
  int header_size = size_varint.Read(header_buffer);

  streaming_read_offset_ = read_offset_ + header_size;
  streaming_read_bytes_remaining_ = size_varint.value();

  read_offset_ += header_size + size_varint.value();
  read_offset_ %= bytes_max();

  return size_varint.value();
}

size_t Ringfile::StreamingRead(void * ptr, size_t size) {
  if (size > streaming_read_bytes_remaining_) {
    size = streaming_read_bytes_remaining_;
  }
  if (size == 0) {
    return 0;
  }
  if (!WrappingRead(streaming_read_offset_, ptr, size)) {
    return -1;
  }

  streaming_read_offset_ += size;
  streaming_read_bytes_remaining_ -= size;
  return size;
}

bool Ringfile::StreamingReadFinish() {
  streaming_read_offset_ = 0;
  return true;
}
