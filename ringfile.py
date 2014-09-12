# Copyright (c) 2014 Ross Kinder. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from cffi import FFI

class Ringfile(object):
  _ffi = FFI()
  _ffi.cdef("""
    struct RINGFILE;
    struct RINGFILE * ringfile_create(const char * path, size_t size);
    struct RINGFILE * ringfile_open(const char * path, const char * mode);
    size_t ringfile_write(const void * ptr, size_t size,
      struct RINGFILE * stream);
    size_t ringfile_read(void * ptr, size_t size, struct RINGFILE * stream);
    size_t ringfile_next_record_size(struct RINGFILE * stream);
    int ringfile_close(struct RINGFILE * stream);
    """)
  if False:
    _library = _ffi.dlopen("src/.libs/libringfile.dylib")
  else:
    _library = _ffi.verify("#include <ringfile.h>",
      libraries=["ringfile"],
      include_dirs=["/tmp/foo/include"],
      library_dirs=["/tmp/foo/lib"])

  def __init__(self, path, mode="r", _size=None):
    if _size is not None:
      self._impl = self._library.ringfile_create(path, _size)
    else:
      self._impl = self._library.ringfile_open(path, mode)
    if self._impl == self._ffi.NULL:
      self._impl = None
      raise IOError()

  def __del__(self):
    if self._impl is not None:
      self._library.ringfile_close(self._impl)

  @classmethod
  def Create(cls, path, size):
    return cls(path, None, _size=size)

  def Write(self, buffer):
    size = self._library.ringfile_write(buffer, len(buffer), self._impl)
    if size != len(buffer):
      raise IOError()

  def Read(self):
    record_size = self._library.ringfile_next_record_size(self._impl)
    if record_size == 0xffffffffffffffffL:
      return None

    buffer = self._ffi.new("char[]", record_size)
    actual_record_size = self._library.ringfile_read(buffer, record_size,
      self._impl)
    if actual_record_size != record_size:
      raise IOError()
    return self._ffi.string(buffer)

  def Close(self):
    self._library.ringfile_close(self._impl)
    self._impl = None
