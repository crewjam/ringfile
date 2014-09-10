#ifndef LIBRINGFILE_RINGFILE_H_
#define LIBRINGFILE_RINGFILE_H_
#include <sys/types.h>
#include <stdint.h>

#include <ringfile.h>

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
#pragma pack(pop)

class Ringfile {
 public:
  Ringfile();
  ~Ringfile();

  bool Open(const char * path, const char * mode);
  bool Fdopen(int fd, const char * mode);
  bool Resize(size_t size);
  bool Write(const void * ptr, size_t size);
  bool Read(void * ptr, size_t size);
  size_t NextRecordSize();
  
  bool Close();
  bool Eof();

 private:
  int fd_;
  bool fd_is_owned_;
  int errno_;
};

struct RINGFILE {
  Ringfile ringfile;
};

#endif  // LIBRINGFILE_RINGFILE_H_



