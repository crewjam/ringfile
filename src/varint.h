// Copyright (c) 2014 Ross Kinder. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef VARINT_H_
#define VARINT_H_

#include <stdint.h>

class Varint {
 public:
  enum {
    kMaxSize = 9
  };

  Varint();
  void set_value(uint64_t value) { value_ = value; }
  uint64_t value() const { return value_; }

  int Read(const void * buffer);

  int ByteSize() const;
  void Write(void * buffer) const;

 private:
  uint64_t value_;
};

#endif  // VARINT_H_
