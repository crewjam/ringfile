#include "ringfile_internal.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

Ringfile::Ringfile() 
  : fd_(-1),
    fd_is_owned_(false) {
}

Ringfile::~Ringfile() {
  if (fd_ != -1 and fd_is_owned_) {
    close(fd_);
  }
}

bool Ringfile::Open(const char * path, const char * mode) {
  if (strcmp(mode, "r") == 0) {
    fd_ = open(path, O_RDONLY);
  } else if (strcmp(mode, "a") == 0) {
    fd_ = open(path, O_RDWR);
  }

  if (fd_ == -1) {
    errno_ = errno;
    return false;
  }
  fd_is_owned_ = true;
  return true;
}

bool Ringfile::Fdopen(int fd, const char * mode) {
  fd_ = fd;
  fd_is_owned_ = false;
  return true;
}

bool Ringfile::Resize(size_t size) {
  return ftruncate(fd_, size) == 0;
}

bool Ringfile::Write(const void * ptr, size_t size) {
  return false;
} 

bool Ringfile::Read(void * ptr, size_t size) {
  return false;
}

size_t Ringfile::NextRecordSize() {
  return -1;
}

bool Ringfile::Close() {
  return true;
}

bool Ringfile::Eof() {
  return false;
}


