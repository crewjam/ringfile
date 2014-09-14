// Copyright (c) 2014 Ross Kinder. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "ringfile_internal.h"

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
  header_->start_offset = sizeof(*header_);
  header_->end_offset = sizeof(*header_);

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

bool Ringfile::PopRecord() {
  if (header_->start_offset == header_->end_offset) {
    // Cannot pop from an empty list
    return false;
  }

  // Read the first record header
  uint64_t offset = header_->start_offset;
  RecordHeader record_header;
  if (!WrappingRead(&offset, &record_header, sizeof(record_header))) {
    return false;
  }

  // Advance the start pointer to the end of the record.
  header_->start_offset += sizeof(record_header) + record_header.size;
  if (header_->start_offset > size_) {
    header_->start_offset -= size_;
    header_->start_offset += sizeof(Header);
  }

  // Reset an empty list (optional)
  if (header_->start_offset == header_->end_offset) {
    header_->start_offset = sizeof(Header);
    header_->end_offset = sizeof(Header);
  }

  return true;
}

bool Ringfile::WrappingWrite(const void * ptr, size_t size) {
  // Write the part of the message that fits at the end, if any.
  int64_t end_bytes_available = size_ - header_->end_offset;
  ssize_t end_bytes_to_write = std::min(static_cast<int64_t>(size),
    end_bytes_available);

  if (end_bytes_to_write) {
    size_t offset_after_write = header_->end_offset + end_bytes_to_write;
    while (offset_after_write > header_->start_offset &&
        header_->start_offset > header_->end_offset) {
      if (!PopRecord()) {
        return false;
      }
    }

    if (lseek(fd_, header_->end_offset, SEEK_SET) == -1) {
      error_ = errno;
      return false;
    }
    if (write(fd_, ptr, end_bytes_to_write) != end_bytes_to_write) {
      error_ = errno;
      return false;
    }
    header_->end_offset = offset_after_write;
  }

  // Write the part of the message at the beginning, if any.
  size_t start_bytes_to_write = size - end_bytes_to_write;
  const void * start_ptr = reinterpret_cast<const char *>(ptr) +
    end_bytes_to_write;
  if (start_bytes_to_write) {
    size_t offset_after_write = sizeof(Header) + start_bytes_to_write;
    while (offset_after_write > header_->start_offset &&
        header_->end_offset > header_->start_offset) {
      PopRecord();
    }

    if (lseek(fd_, sizeof(Header), SEEK_SET) == -1) {
      error_ = errno;
      return false;
    }
    if (write(fd_, start_ptr, start_bytes_to_write) != start_bytes_to_write) {
      error_ = errno;
      return false;
    }
    header_->end_offset = offset_after_write;
  }
  return true;
}

bool Ringfile::Write(const void * ptr, size_t size) {
  // Refuse to write a message that won't fit completely in the file
  if (size > size_ - sizeof(Header) - sizeof(RecordHeader)) {
    error_ = EINVAL;
    return false;
  }

  uint32_t record_header = size;
  if (!WrappingWrite(&record_header, sizeof(record_header))) {
    return false;
  }
  if (!WrappingWrite(ptr, size)) {
    return false;
  }
  return true;
}

bool Ringfile::WrappingRead(uint64_t * offset, void * ptr, size_t size) {
  int64_t end_bytes_available = size_ - *offset;
  int64_t end_bytes_to_read = std::min(static_cast<int64_t>(size),
    end_bytes_available);

  if (end_bytes_to_read) {
    if (lseek(fd_, *offset, SEEK_SET) == -1) {
      error_ = errno;
      return false;
    }
    if (read(fd_, ptr, end_bytes_to_read) != end_bytes_to_read) {
      error_ = errno;
      return false;
    }
    *offset += size;
  }

  ssize_t start_bytes_to_read = size - end_bytes_to_read;
  if (start_bytes_to_read) {
    if (lseek(fd_, sizeof(Header), SEEK_SET) == -1) {
      error_ = errno;
      return false;
    }
    if (read(fd_, reinterpret_cast<char *>(ptr) + end_bytes_to_read,
        start_bytes_to_read) != start_bytes_to_read) {
      error_ = errno;
      return false;
    }
    *offset = sizeof(Header) + start_bytes_to_read;
  }
  return true;
}

size_t Ringfile::Read(void * buffer, size_t buffer_size) {
  if (!header_ || fd_ == -1) {
    return -1;
  }
  uint64_t starting_read_offset = read_offset_;

  RecordHeader record_header;
  if (!WrappingRead(&read_offset_, &record_header, sizeof(record_header))) {
    read_offset_ = starting_read_offset;
    return -1;
  }
  if (record_header.size > buffer_size) {
    read_offset_ = starting_read_offset;
    return -1;
  }
  if (!WrappingRead(&read_offset_, buffer, record_header.size)) {
    return -1;
  }
  return record_header.size;
}

size_t Ringfile::NextRecordSize() {
  if (!header_ || fd_ == -1) {
    return -1;
  }
  if (read_offset_ == header_->end_offset) {
    return -1;
  }

  uint64_t read_offset = read_offset_;
  RecordHeader record_header;
  if (!WrappingRead(&read_offset, &record_header, sizeof(record_header))) {
    return -1;
  }
  return record_header.size;
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

bool Ringfile::Eof() {
  if (!header_ || fd_ == -1) {
    return false;
  }
  return header_->end_offset == lseek(fd_, 0, SEEK_CUR);
}

int Ringfile::size_max() const {
  return size_;
}

int Ringfile::size_used() const {
  if (header_->start_offset <= header_->end_offset) {
    return header_->end_offset - header_->start_offset;
  } else {
    return size_ - (header_->start_offset - header_->end_offset);
  }
}


bool Ringfile::StreamingWriteStart() {
  partial_write_header_.size = 0;
  partial_write_header_offset_ = header_->end_offset;
  if (!WrappingWrite(&partial_write_header_, sizeof(partial_write_header_))) {
    return false;
  }
  return true;
}

bool Ringfile::StreamingWrite(const void * ptr, size_t size) {
  partial_write_header_.size += size;

  // Refuse to write a message that won't fit completely in the file
  if (partial_write_header_.size > size_ - sizeof(Header) -
      sizeof(RecordHeader)) {
    error_ = EINVAL;
    header_->end_offset = partial_write_header_offset_;
    return false;
  }

  if (!WrappingWrite(ptr, size)) {
    return false;
  }
  return true;
}

bool Ringfile::StreamingWriteFinish() {
  return WrappingWriteAtOffset(partial_write_header_offset_,
    &partial_write_header_, sizeof(partial_write_header_));
}

bool Ringfile::WrappingWriteAtOffset(uint64_t offset, const void * data,
    size_t size) {
  int start_bytes_to_write = 0;
  int end_bytes_to_write = size;

  int end_offset = offset + size;
  if (end_offset >= size_) {
    start_bytes_to_write += (end_offset - size_);
    end_bytes_to_write -= start_bytes_to_write;
    end_offset = sizeof(Header) + start_bytes_to_write;
  }

  if (end_bytes_to_write) {
    if (lseek(fd_, offset, SEEK_SET) == -1) {
      error_ = errno;
      return false;
    }
    if (write(fd_, data, end_bytes_to_write) != end_bytes_to_write) {
      error_ = errno;
      return false;
    }
  }

  if (start_bytes_to_write) {
    if (lseek(fd_, sizeof(Header), SEEK_SET) == -1) {
      error_ = errno;
      return false;
    }
    if (write(fd_, reinterpret_cast<const char *>(data) + end_bytes_to_write,
        start_bytes_to_write) != start_bytes_to_write) {
      error_ = errno;
      return false;
    }
  }

  return true;
}
