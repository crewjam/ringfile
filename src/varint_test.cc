// Copyright (c) 2014 Ross Kinder. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <gtest/gtest.h>

#include "varint.h"
#include "test_util.h"

struct VarintTestCase {
  uint64_t value;
  int size;
  uint8_t encoded[10];
};

// Note: these test vectors for varint encoding come from the Google Protocol
//   buffer source.
VarintTestCase kVarintTestCases[] = {
  {0, 1, {0x00}},
  {1, 1, {0x01}},
  {127, 1, {0x7f}},
  {14882, 2, {0xa2, 0x74}},
  {2961488830ULL, 5, {0xbe, 0xf7, 0x92, 0x84, 0x0b}},
  {7256456126ULL, 5, {0xbe, 0xf7, 0x92, 0x84, 0x1b}},
  {41256202580718336ULL, 8, {0x80, 0xe6, 0xeb, 0x9c, 0xc3, 0xc9, 0xa4, 0x49}},
  {11964378330978735131ULL, 10,
    {0x9b, 0xa8, 0xf9, 0xc2, 0xbb, 0xd6, 0x80, 0x85, 0xa6, 0x01}},
};

class VarintTest : public ::testing::TestWithParam<VarintTestCase> {
};

TEST_P(VarintTest, Read) {
  uint8_t buffer[10];
  memset(&buffer, 0xfe, 10);
  memcpy(&buffer, GetParam().encoded, GetParam().size);

  Varint v;
  EXPECT_EQ(GetParam().size, v.Read(buffer));
  EXPECT_EQ(GetParam().value, v.value());
}

TEST_P(VarintTest, Write) {
  uint8_t buffer_expected[10];
  memset(&buffer_expected, 0xfe, 10);
  memcpy(&buffer_expected, GetParam().encoded, GetParam().size);


  uint8_t buffer[10];
  memset(&buffer, 0xfe, 10);

  Varint v;
  v.set_value(GetParam().value);
  EXPECT_EQ(GetParam().size, v.ByteSize());
  v.Write(&buffer);
  EXPECT_EQ(0, memcmp(buffer, buffer_expected, 10))
    << ::testing::PrintToString(buffer);
}

INSTANTIATE_TEST_CASE_P(Run, VarintTest, ::testing::ValuesIn(kVarintTestCases));
