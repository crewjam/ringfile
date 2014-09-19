// Copyright (c) 2014 Ross Kinder. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "varint.h"

#include <math.h>

int Varint::Read(const void * buffer) {
  value_ = 0;
  for (int shift = 0; shift < 10; ++shift) {
    uint8_t byte = *(reinterpret_cast<const uint8_t *>(buffer) + shift);
    value_ |= (static_cast<uint64_t>(byte & 0x7f) << (shift * 7));
    if ((byte & 0x80) == 0) {
      return shift + 1;
    }
  }
}

int Varint::ByteSize() const {
  for (int shift = 0; shift < 10; ++shift) {
    if (value_ < exp2((shift + 1) * 7)) {
      return shift + 1;
    }
  }
}

void Varint::Write(void * buffer) const {
  for (int shift = 0; shift < 10; ++shift) {
    uint8_t * byte_ptr = reinterpret_cast<uint8_t *>(buffer) + shift;

    uint8_t marker = 0x80;
    if (value_ < exp2((shift + 1) * 7)) {
      marker = 0;
    }

    *byte_ptr = ((value_ >> (shift * 7)) & 0x7f) | marker;

    if (!marker) {
      break;
    }
  }
}
