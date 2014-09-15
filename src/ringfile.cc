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

Ringfile::Ringfile()
  : fd_(-1),
    fd_is_owned_(false),
    header_(NULL) {
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
  RecordHeader record_header;
  if (!WrappingRead(header_->start_offset, &record_header,
      sizeof(record_header))) {
    return false;
  }
  
  // Advance the start pointer to the end of the record.
  header_->start_offset += sizeof(record_header) + record_header.size;
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

  RecordHeader record_header;
  if (!WrappingRead(read_offset_, &record_header, sizeof(record_header))) {
    return false;
  }
  *size = record_header.size;
  return true;
}

bool Ringfile::Read(void * buffer, size_t buffer_size) {
  if (!header_ || fd_ == -1) {
    return false;
  }
  
  RecordHeader record_header;
  if (!WrappingRead(read_offset_, &record_header, sizeof(record_header))) {
    return false;
  }
  if (record_header.size > buffer_size) {
    return false;
  }
  if (!WrappingRead(read_offset_ + sizeof(record_header), buffer, 
      record_header.size)) {
    return false;
  }
  read_offset_ += sizeof(record_header) + record_header.size;
  read_offset_ %= bytes_max();
  return true;
}

bool Ringfile::Write(const void * ptr, size_t size) {
  RecordHeader record_header;
  record_header.size = size;
  
  // Refuse a record that is too big for the buffer
  if (bytes_max() < (sizeof(record_header) + size + 1)) {
    return false;
  }
  
  // Pop records until there is enough space available
  while (bytes_available() < (sizeof(record_header) + size)) {
    size_t bytes_available_start =bytes_available();
    if (!PopRecord()) {
      return false;
    }
    assert(bytes_available() > bytes_available_start);
  }
  
  if (!WrappingWrite(header_->end_offset, &record_header, 
      sizeof(record_header))) {
    return false;
  }
  if (!WrappingWrite(header_->end_offset + sizeof(record_header), ptr, size)) {
    return false;
  }
  
  header_->end_offset += sizeof(record_header) + size;
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


bool Ringfile::StreamingWriteStart() {
  // We need enough space in the buffer for the header only
  while (bytes_available() < sizeof(partial_write_header_)) {
    PopRecord();
  }
  
  partial_write_header_.size = 0;
  partial_write_header_offset_ = header_->end_offset;
  
  if (!WrappingWrite(header_->end_offset, 
      &partial_write_header_, sizeof(partial_write_header_))) {
    return false;
  }
  return true;
}

bool Ringfile::StreamingWrite(const void * ptr, size_t size) {
  uint32_t new_record_size = partial_write_header_.size + size;
  
  // Refuse to write a message that won't fit completely in the file
  if (sizeof(partial_write_header_) + new_record_size > bytes_max() - 1) {
    error_ = EINVAL;
    return false;
  }
  
  // We need enough space for the header plus as much space as the record takes
  // so far.
  while (bytes_available() < sizeof(partial_write_header_) + new_record_size) {
    PopRecord();
  }
  
  // Write this part
  uint64_t offset = header_->end_offset + sizeof(partial_write_header_) + \
    partial_write_header_.size;
  if (!WrappingWrite(offset, ptr, size)) {
    return false;
  }
  
  partial_write_header_.size = new_record_size;
  return true;
}

bool Ringfile::StreamingWriteFinish() {
  // Write the message header at the saved offset 
  if (!WrappingWrite(header_->end_offset, &partial_write_header_, 
      sizeof(partial_write_header_))) {
    return false;
  }
  header_->end_offset += sizeof(partial_write_header_) + 
    partial_write_header_.size;
  header_->end_offset %= bytes_max();
  return true;
}

size_t Ringfile::StreamingReadStart() {
  if (!header_ || fd_ == -1) {
    return -1;
  }
  if (read_offset_ == header_->end_offset) {
    return -1;
  }
  
  if (!WrappingRead(read_offset_, &partial_read_header_, 
      sizeof(partial_read_header_))) {
    return -1;
  }
  partial_read_offset_ = 0;
  
  return partial_read_header_.size;
}

size_t Ringfile::StreamingRead(void * ptr, size_t size) {
  uint64_t offset = read_offset_ + sizeof(partial_read_header_) + 
    partial_read_offset_;
  if (partial_read_offset_ + size > partial_read_header_.size) {
    size = partial_read_header_.size - partial_read_offset_;
  }
  if (size == 0) {
    return 0;
  }
  
  if (!WrappingRead(offset, ptr, size)) {
    return -1;
  }
  
  partial_read_offset_ += size;
  return size;
}

bool Ringfile::StreamingReadFinish() {
  read_offset_ += sizeof(partial_read_header_) + partial_read_offset_;
  read_offset_ %= bytes_max();
  return true;
}
