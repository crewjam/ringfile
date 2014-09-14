// Copyright (c) 2014 Ross Kinder. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <errno.h>
#include <gtest/gtest.h>

#include "ringfile_internal.h"
#include "test_util.h"

TEST(RingfileTest, CannotOpenBogusPath) {
  std::string path = TempDir() + "/does not exist/ring";
  Ringfile ringfile;
  ASSERT_FALSE(ringfile.Create(path, 1024));
  ASSERT_EQ(ENOENT, ringfile.error());
}

TEST(RingfileTest, CanReadAndWriteBasics) {
  std::string path = TempDir() + "/ring";

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Create(path, 1024));

    std::string message("Hello, World!");
    ringfile.Write(message.c_str(), message.size());
    ringfile.Close();
  }

  EXPECT_EQ(std::string(
    "RING"  // magic number
    "\x00\x00\x00\x00"  // flags
    "\x18\x00\x00\x00\x00\x00\x00\x00"  // start offset
    "\x29\x00\x00\x00\x00\x00\x00\x00"  // end offset
    "\x0d\x00\x00\x00"  // record length
    "Hello, World!", 41), GetFileContents(path).substr(0, 41));

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Open(path, Ringfile::kRead))
      << strerror(ringfile.error());

    int next_record_size = ringfile.NextRecordSize();
    EXPECT_EQ(13, next_record_size) << strerror(ringfile.error());

    std::string buffer;
    buffer.resize(next_record_size);
    EXPECT_TRUE(ringfile.Read(const_cast<char *>(buffer.c_str()),
      next_record_size));
    EXPECT_EQ("Hello, World!", buffer);

    next_record_size = ringfile.NextRecordSize();
    EXPECT_EQ(-1, next_record_size);
  }
}

// This test checks that a write can extend beyond the end of the file and wrap
// around to the beginning
//
// Before:
//
//    0        24            41            50
//   +--------+-------------+--------------+
//   | header | record1 17b |              |
//   +--------+-------------+--------------+
//
// After:
//    0        24    32        41            50
//   +--------+-----+--------+--------------+
//   | header |...r2|        | record2 ...  |
//   +--------+-----+--------+--------------+
//
TEST(RingfileTest, CircularWrite) {
  std::string path = TempDir() + "/ring";

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Create(path, 50));

    std::string message("Hello, World!");
    ringfile.Write(message.c_str(), message.size());
    ringfile.Close();
  }

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Open(path, Ringfile::kAppend));

    std::string message("Goodbye, Bob!");
    ringfile.Write(message.c_str(), message.size());
    ringfile.Close();
  }

  EXPECT_EQ(std::string(
    "RING"  // magic number
    "\x00\x00\x00\x00"  // flags
    "\x29\x00\x00\x00\x00\x00\x00\x00"  // start offset
    "\x20\x00\x00\x00\x00\x00\x00\x00"  // end offset
    "ye, Bob!"  // end of the record
    "o, World!"  // leftover from hello world
    "\x0d\x00\x00\x00Goodb"  // beginning of message
    , 50), GetFileContents(path));

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Open(path, Ringfile::kRead))
      << strerror(ringfile.error());

    int next_record_size = ringfile.NextRecordSize();
    EXPECT_EQ(13, next_record_size) << strerror(ringfile.error());

    std::string buffer;
    buffer.resize(next_record_size);
    EXPECT_TRUE(ringfile.Read(const_cast<char *>(buffer.c_str()),
      next_record_size));
    EXPECT_EQ("Goodbye, Bob!", buffer);

    next_record_size = ringfile.NextRecordSize();
    EXPECT_EQ(-1, next_record_size);
  }
}

// This test checks that a write can work when the end offset is < start offset
TEST(RingfileTest, ReverseCircularWrite) {
  std::string path = TempDir() + "/ring";

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Create(path, 50));

    std::string message("0123456789abc");
    ASSERT_TRUE(ringfile.Write(message.c_str(), message.size()));

    message = "ABCDEFGHIJKLM";
    ASSERT_TRUE(ringfile.Write(message.c_str(), message.size()));

    message = "nopqrstuvwxyz";
    ASSERT_TRUE(ringfile.Write(message.c_str(), message.size()));

    ringfile.Close();
  }

  EXPECT_EQ(std::string(
    "RING"  // magic number
    "\x00\x00\x00\x00"  // flags
    "\x20\x00\x00\x00\x00\x00\x00\x00"  // start offset
    "\x31\x00\x00\x00\x00\x00\x00\x00"  // end offset
    "FGHIJKLM"  // leftover from record 2
    "\x0d\x00\x00\x00nopqrstuvwxyz"  // record 3
    "E"  // trailing byte (??)
    , 50), GetFileContents(path));

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Open(path, Ringfile::kRead))
      << strerror(ringfile.error());

    int next_record_size = ringfile.NextRecordSize();
    EXPECT_EQ(13, next_record_size) << strerror(ringfile.error());

    std::string buffer;
    buffer.resize(next_record_size);
    EXPECT_TRUE(ringfile.Read(const_cast<char *>(buffer.c_str()),
      next_record_size));
    EXPECT_EQ("nopqrstuvwxyz", buffer);

    next_record_size = ringfile.NextRecordSize();
    EXPECT_EQ(-1, next_record_size);
  }
}

// This test checks that a write can work for the maximum size message but
// not for a larger one
TEST(RingfileTest, CannotWriteMessageAboveLimit) {
  std::string path = TempDir() + "/ring";

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Create(path, 50));

    // The maximum message size is 50 - 24 - 4 = 22 bytes
    std::string message("01234567890123456789012");  // 23 bytes
    ASSERT_FALSE(ringfile.Write(message.c_str(), message.size()));

    message = "0123456789012345678901";  // 22 bytes
    ASSERT_TRUE(ringfile.Write(message.c_str(), message.size()));
    ringfile.Close();
  }

  EXPECT_EQ(std::string(
    "RING"  // magic number
    "\x00\x00\x00\x00"  // flags
    "\x18\x00\x00\x00\x00\x00\x00\x00"  // start offset
    "\x32\x00\x00\x00\x00\x00\x00\x00"  // end offset
    "\x16\x00\x00\x00" "0123456789012345678901"  // record
    , 50), GetFileContents(path));

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Open(path, Ringfile::kRead))
      << strerror(ringfile.error());

    int next_record_size = ringfile.NextRecordSize();
    EXPECT_EQ(22, next_record_size) << strerror(ringfile.error());

    std::string buffer;
    buffer.resize(next_record_size);
    EXPECT_TRUE(ringfile.Read(const_cast<char *>(buffer.c_str()),
      next_record_size));
    EXPECT_EQ("0123456789012345678901", buffer);

    next_record_size = ringfile.NextRecordSize();
    EXPECT_EQ(-1, next_record_size);
  }
}

// This test checks that the write can work when the offset is at the end of
// the file.
TEST(RingfileTest, CanAppendEdge) {
  std::string path = TempDir() + "/ring";

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Create(path, 50));

    // The maximum message size is 50 - 24 - 4 = 22 bytes
    std::string message("0123456789012345678901");
    ASSERT_TRUE(ringfile.Write(message.c_str(), message.size()));

    message = "abcd";
    ASSERT_TRUE(ringfile.Write(message.c_str(), message.size()));

    ringfile.Close();
  }

  EXPECT_EQ(std::string(
    "RING"  // magic number
    "\x00\x00\x00\x00"  // flags
    "\x18\x00\x00\x00\x00\x00\x00\x00"  // start offset
    "\x20\x00\x00\x00\x00\x00\x00\x00"  // end offset
    "\x04\x00\x00\x00" "abcd"  // record
    "456789012345678901"  // old record
    , 50), GetFileContents(path));

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Open(path, Ringfile::kRead))
      << strerror(ringfile.error());

    int next_record_size = ringfile.NextRecordSize();
    EXPECT_EQ(4, next_record_size) << strerror(ringfile.error());

    std::string buffer;
    buffer.resize(next_record_size);
    EXPECT_TRUE(ringfile.Read(const_cast<char *>(buffer.c_str()),
      next_record_size));
    EXPECT_EQ("abcd", buffer);

    next_record_size = ringfile.NextRecordSize();
    EXPECT_EQ(-1, next_record_size);
  }
}

// This test checks that a write can work when the end offset is < start offset
TEST(RingfileTest, ReverseCircularWriteStreaming) {
  std::string path = TempDir() + "/ring";

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Create(path, 50));

    std::string message("0123456789abc");
    ASSERT_TRUE(ringfile.StreamingWriteStart());
    ASSERT_TRUE(ringfile.StreamingWrite(message.c_str(), message.size()));
    ASSERT_TRUE(ringfile.StreamingWriteFinish());

    message = "ABCDEFGHIJKLM";
    ASSERT_TRUE(ringfile.StreamingWriteStart());
    ASSERT_TRUE(ringfile.StreamingWrite(message.c_str(), message.size()));
    ASSERT_TRUE(ringfile.StreamingWriteFinish());

    message = "nopqrstuvwxyz";
    ASSERT_TRUE(ringfile.StreamingWriteStart());
    ASSERT_TRUE(ringfile.StreamingWrite(message.c_str(), message.size()));
    ASSERT_TRUE(ringfile.StreamingWriteFinish());

    ringfile.Close();
  }

  EXPECT_EQ(std::string(
    "RING"  // magic number
    "\x00\x00\x00\x00"  // flags
    "\x20\x00\x00\x00\x00\x00\x00\x00"  // start offset
    "\x31\x00\x00\x00\x00\x00\x00\x00"  // end offset
    "FGHIJKLM"  // leftover from record 2
    "\x0d\x00\x00\x00nopqrstuvwxyz"  // record 3
    "E"  // trailing byte (??)
    , 50), GetFileContents(path));

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Open(path, Ringfile::kRead))
      << strerror(ringfile.error());

    int next_record_size = ringfile.NextRecordSize();
    EXPECT_EQ(13, next_record_size) << strerror(ringfile.error());

    std::string buffer;
    buffer.resize(next_record_size);
    EXPECT_TRUE(ringfile.Read(const_cast<char *>(buffer.c_str()),
      next_record_size));
    EXPECT_EQ("nopqrstuvwxyz", buffer);

    next_record_size = ringfile.NextRecordSize();
    EXPECT_EQ(-1, next_record_size);
  }
}

// This test checks that a write can work for the maximum size message but
// not for a larger one
TEST(RingfileTest, CannotWriteMessageAboveLimitStreaming) {
  std::string path = TempDir() + "/ring";

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Create(path, 50));

    // The maximum message size is 50 - 24 - 4 = 22 bytes
    std::string message("01234567890123456789012");  // 23 bytes
    ASSERT_TRUE(ringfile.StreamingWriteStart());
    ASSERT_FALSE(ringfile.StreamingWrite(message.c_str(), message.size()));
    ASSERT_TRUE(ringfile.StreamingWriteFinish());

    message = "0123456789012345678901";  // 22 bytes
    ASSERT_TRUE(ringfile.StreamingWriteStart());
    ASSERT_TRUE(ringfile.StreamingWrite(message.c_str(), message.size()));
    ASSERT_TRUE(ringfile.StreamingWriteFinish());
    ringfile.Close();
  }

  EXPECT_EQ(std::string(
    "RING"  // magic number
    "\x00\x00\x00\x00"  // flags
    "\x18\x00\x00\x00\x00\x00\x00\x00"  // start offset
    "\x32\x00\x00\x00\x00\x00\x00\x00"  // end offset
    "\x16\x00\x00\x00" "0123456789012345678901"  // record
    , 50), GetFileContents(path));

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Open(path, Ringfile::kRead))
      << strerror(ringfile.error());

    int next_record_size = ringfile.NextRecordSize();
    EXPECT_EQ(22, next_record_size) << strerror(ringfile.error());

    std::string buffer;
    buffer.resize(next_record_size);
    EXPECT_TRUE(ringfile.Read(const_cast<char *>(buffer.c_str()),
      next_record_size));
    EXPECT_EQ("0123456789012345678901", buffer);

    next_record_size = ringfile.NextRecordSize();
    EXPECT_EQ(-1, next_record_size);
  }
}

// This test checks that the write can work when the offset is at the end of
// the file.
TEST(RingfileTest, CanAppendEdgeStreaming) {
  std::string path = TempDir() + "/ring";

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Create(path, 50));

    // The maximum message size is 50 - 24 - 4 = 22 bytes
    std::string message("0123456789012345678901");
    ASSERT_TRUE(ringfile.StreamingWriteStart());
    ASSERT_TRUE(ringfile.StreamingWrite(message.c_str(), message.size()));
    ASSERT_TRUE(ringfile.StreamingWriteFinish())
      << strerror(ringfile.error());

    message = "abcd";
    ASSERT_TRUE(ringfile.StreamingWriteStart());
    ASSERT_TRUE(ringfile.StreamingWrite(message.c_str(), message.size()));
    ASSERT_TRUE(ringfile.StreamingWriteFinish())
      << strerror(ringfile.error());

    ringfile.Close();
  }

  EXPECT_EQ(std::string(
    "RING"  // magic number
    "\x00\x00\x00\x00"  // flags
    "\x18\x00\x00\x00\x00\x00\x00\x00"  // start offset
    "\x20\x00\x00\x00\x00\x00\x00\x00"  // end offset
    "\x04\x00\x00\x00" "abcd"  // record
    "456789012345678901"  // old record
    , 50), GetFileContents(path));

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Open(path, Ringfile::kRead))
      << strerror(ringfile.error());

    int next_record_size = ringfile.NextRecordSize();
    EXPECT_EQ(4, next_record_size) << strerror(ringfile.error());

    std::string buffer;
    buffer.resize(next_record_size);
    EXPECT_TRUE(ringfile.Read(const_cast<char *>(buffer.c_str()),
      next_record_size));
    EXPECT_EQ("abcd", buffer);

    next_record_size = ringfile.NextRecordSize();
    EXPECT_EQ(-1, next_record_size);
  }
}

