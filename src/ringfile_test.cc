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
    "\x00\x00\x00\x00\x00\x00\x00\x00"  // start offset
    "\x0e\x00\x00\x00\x00\x00\x00\x00"  // end offset
    "\x0d"  // record length
    "Hello, World!", 38), GetFileContents(path).substr(0, 38));

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Open(path, Ringfile::kRead))
      << strerror(ringfile.error());

    size_t next_record_size;
    EXPECT_TRUE(ringfile.NextRecordSize(&next_record_size));
    EXPECT_EQ(13, next_record_size) << strerror(ringfile.error());

    std::string buffer;
    buffer.resize(next_record_size);
    EXPECT_TRUE(ringfile.Read(const_cast<char *>(buffer.c_str()),
      next_record_size));
    EXPECT_EQ("Hello, World!", buffer);

    EXPECT_FALSE(ringfile.NextRecordSize(NULL));
  }
}

TEST(RingfileTest, CanReadAndWriteBasicsStreaming) {
  std::string path = TempDir() + "/ring";

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Create(path, 1024));

    ASSERT_EQ(true, ringfile.StreamingWriteStart(13));
    ASSERT_EQ(true, ringfile.StreamingWrite("Hello, ", 7));
    ASSERT_EQ(true, ringfile.StreamingWrite("World!", 6));
    ASSERT_EQ(true, ringfile.StreamingWriteFinish());
    ringfile.Close();
  }

  EXPECT_EQ(std::string(
    "RING"  // magic number
    "\x00\x00\x00\x00"  // flags
    "\x00\x00\x00\x00\x00\x00\x00\x00"  // start offset
    "\x0e\x00\x00\x00\x00\x00\x00\x00"  // end offset
    "\x0d"  // record length
    "Hello, World!", 38), GetFileContents(path).substr(0, 38));

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Open(path, Ringfile::kRead))
      << strerror(ringfile.error());

    EXPECT_EQ(13, ringfile.StreamingReadStart());

    char buffer[6];
    buffer[5] = 0;
    EXPECT_EQ(5, ringfile.StreamingRead(&buffer, 5));
    EXPECT_STREQ("Hello", buffer);

    EXPECT_EQ(5, ringfile.StreamingRead(&buffer, 5));
    EXPECT_STREQ(", Wor", buffer);

    buffer[3] = 0;
    EXPECT_EQ(3, ringfile.StreamingRead(&buffer, 5));
    EXPECT_STREQ("ld!", buffer);

    EXPECT_EQ(true, ringfile.StreamingReadFinish());

    EXPECT_EQ(-1, ringfile.StreamingReadStart());
    EXPECT_FALSE(ringfile.NextRecordSize(NULL));
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

    std::string message("abcHello, World!");
    ringfile.Write(message.c_str(), message.size());
    ringfile.Close();
  }

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Open(path, Ringfile::kAppend));

    std::string message("defGoodbye, Bob!");
    ringfile.Write(message.c_str(), message.size());
    ringfile.Close();
  }

  EXPECT_EQ(std::string(
    "RING"  // magic number
    "\x00\x00\x00\x00"  // flags
    "\x11\x00\x00\x00\x00\x00\x00\x00"  // start offset
    "\x08\x00\x00\x00\x00\x00\x00\x00"  // end offset
    "ye, Bob!"  // end of the record
    "o, World!"  // leftover from hello world
    "\x10" "defGoodb"  // beginning of message
    , 50), GetFileContents(path));

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Open(path, Ringfile::kRead))
      << strerror(ringfile.error());

    size_t next_record_size;
    EXPECT_TRUE(ringfile.NextRecordSize(&next_record_size));
    EXPECT_EQ(16, next_record_size) << strerror(ringfile.error());

    std::string buffer;
    buffer.resize(next_record_size);
    EXPECT_TRUE(ringfile.Read(const_cast<char *>(buffer.c_str()),
      next_record_size));
    EXPECT_EQ("defGoodbye, Bob!", buffer);

    EXPECT_FALSE(ringfile.NextRecordSize(NULL));
  }
}

TEST(RingfileTest, CircularWriteStreaming) {
  std::string path = TempDir() + "/ring";

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Create(path, 50));

    ASSERT_EQ(true, ringfile.StreamingWriteStart(16));
    ASSERT_EQ(true, ringfile.StreamingWrite("abc", 3));
    ASSERT_EQ(true, ringfile.StreamingWrite("Hello, ", 7));
    ASSERT_EQ(true, ringfile.StreamingWrite("World!", 6));
    ASSERT_EQ(true, ringfile.StreamingWriteFinish());
    ringfile.Close();
  }

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Open(path, Ringfile::kAppend));

    std::string message("Goodbye, Bob!");
    ASSERT_EQ(true, ringfile.StreamingWriteStart(16));
    ASSERT_EQ(true, ringfile.StreamingWrite("def", 3));
    ASSERT_EQ(true, ringfile.StreamingWrite("Goodbye", 7));
    ASSERT_EQ(true, ringfile.StreamingWrite(", Bob!", 6));
    ASSERT_EQ(true, ringfile.StreamingWriteFinish());

    ringfile.Close();
  }

  EXPECT_EQ(std::string(
    "RING"  // magic number
    "\x00\x00\x00\x00"  // flags
    "\x11\x00\x00\x00\x00\x00\x00\x00"  // start offset
    "\x08\x00\x00\x00\x00\x00\x00\x00"  // end offset
    "ye, Bob!"  // end of the record
    "o, World!"  // leftover from hello world
    "\x10" "defGoodb"  // beginning of message
    , 50), GetFileContents(path));

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Open(path, Ringfile::kRead))
      << strerror(ringfile.error());

    EXPECT_EQ(16, ringfile.StreamingReadStart());

    char buffer[5];
    EXPECT_EQ(5, ringfile.StreamingRead(&buffer, 5));
    EXPECT_STREQ("defGo", buffer);

    EXPECT_EQ(5, ringfile.StreamingRead(&buffer, 5));
    EXPECT_STREQ("odbye", buffer);

    EXPECT_EQ(5, ringfile.StreamingRead(&buffer, 5));
    EXPECT_STREQ(", Bob", buffer);

    buffer[1] = 0;
    EXPECT_EQ(1, ringfile.StreamingRead(&buffer, 5));
    EXPECT_STREQ("!", buffer);

    EXPECT_EQ(true, ringfile.StreamingReadFinish());

    EXPECT_EQ(-1, ringfile.StreamingReadStart());
    EXPECT_FALSE(ringfile.NextRecordSize(NULL));
  }
}

// This test checks that a write can work when the end offset is < start offset
TEST(RingfileTest, ReverseCircularWrite) {
  std::string path = TempDir() + "/ring";

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Create(path, 50));

    std::string message("0123456789abcdef");
    ASSERT_TRUE(ringfile.Write(message.c_str(), message.size()));

    message = "ABCDEFGHIJKLMNOP";
    ASSERT_TRUE(ringfile.Write(message.c_str(), message.size()));

    message = "nopqrstuvwxyz#$%";
    ASSERT_TRUE(ringfile.Write(message.c_str(), message.size()));

    ringfile.Close();
  }

  EXPECT_EQ(std::string(
    "RING"  // magic number
    "\x00\x00\x00\x00"  // flags
    "\x08\x00\x00\x00\x00\x00\x00\x00"  // start offset
    "\x19\x00\x00\x00\x00\x00\x00\x00"  // end offset
    "IJKLMNOP"  // leftover from record 2
    "\x10nopqrstuvwxyz#$%"  // record 3
    "H"  // trailing byte (??)
    , 50), GetFileContents(path));

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Open(path, Ringfile::kRead))
      << strerror(ringfile.error());

    size_t next_record_size;
    EXPECT_TRUE(ringfile.NextRecordSize(&next_record_size));
    EXPECT_EQ(16, next_record_size) << strerror(ringfile.error());

    std::string buffer;
    buffer.resize(next_record_size);
    EXPECT_TRUE(ringfile.Read(const_cast<char *>(buffer.c_str()),
      next_record_size));
    EXPECT_EQ("nopqrstuvwxyz#$%", buffer);

    EXPECT_FALSE(ringfile.NextRecordSize(NULL));
  }
}

// This test checks that a write can work when the end offset is < start offset
TEST(RingfileTest, ReverseCircularWriteStreaming) {
  std::string path = TempDir() + "/ring";

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Create(path, 50));

    std::string message("0123456789abcdef");
    ASSERT_TRUE(ringfile.StreamingWriteStart(message.size()));
    ASSERT_TRUE(ringfile.StreamingWrite(message.c_str(), message.size()));
    ASSERT_TRUE(ringfile.StreamingWriteFinish());

    message = "ABCDEFGHIJKLMNOP";
    ASSERT_TRUE(ringfile.StreamingWriteStart(message.size()));
    ASSERT_TRUE(ringfile.StreamingWrite(message.c_str(), message.size()));
    ASSERT_TRUE(ringfile.StreamingWriteFinish());

    message = "nopqrstuvwxyz#$%";
    ASSERT_TRUE(ringfile.StreamingWriteStart(message.size()));
    ASSERT_TRUE(ringfile.StreamingWrite(message.c_str(), message.size()));
    ASSERT_TRUE(ringfile.StreamingWriteFinish());

    ringfile.Close();
  }

  EXPECT_EQ(std::string(
    "RING"  // magic number
    "\x00\x00\x00\x00"  // flags
    "\x08\x00\x00\x00\x00\x00\x00\x00"  // start offset
    "\x19\x00\x00\x00\x00\x00\x00\x00"  // end offset
    "IJKLMNOP"  // leftover from record 2
    "\x10nopqrstuvwxyz#$%"  // record 3
    "H"  // trailing byte (??)
    , 50), GetFileContents(path));

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Open(path, Ringfile::kRead))
      << strerror(ringfile.error());

    std::string buffer;
    EXPECT_EQ(16, ringfile.StreamingReadStart());
    while (true) {
      char buffer_part[5];
      size_t buffer_part_size = ringfile.StreamingRead(buffer_part, 5);
      if (buffer_part_size == 0) {
        break;
      }
      ASSERT_NE(static_cast<size_t>(-1), buffer_part_size);
      buffer.append(std::string(buffer_part, buffer_part_size));
    }
    ringfile.StreamingReadFinish();
    EXPECT_EQ("nopqrstuvwxyz#$%", buffer);

    EXPECT_EQ(-1, ringfile.StreamingReadStart());
  }
}

// This test checks that a write can work for the maximum size message but
// not for a larger one
TEST(RingfileTest, CannotWriteMessageAboveLimit) {
  std::string path = TempDir() + "/ring";

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Create(path, 50));

    // The maximum record size including header is 50 - 24 - 1 = 25 bytes
    // Which is a 1-byte header and a 24 byte message
    std::string message("0123456789012345678901234");  // 25 bytes
    ASSERT_FALSE(ringfile.Write(message.c_str(), message.size()));

    message = "012345678901234567890123";  // 24 bytes
    ASSERT_TRUE(ringfile.Write(message.c_str(), message.size()));
    ringfile.Close();
  }

  EXPECT_EQ(std::string(
    "RING"  // magic number
    "\x00\x00\x00\x00"  // flags
    "\x00\x00\x00\x00\x00\x00\x00\x00"  // start offset
    "\x19\x00\x00\x00\x00\x00\x00\x00"  // end offset
    "\x18" "012345678901234567890123"  // record
    "\x00"  // unspecified
    , 50), GetFileContents(path));

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Open(path, Ringfile::kRead))
      << strerror(ringfile.error());

    size_t next_record_size;
    EXPECT_TRUE(ringfile.NextRecordSize(&next_record_size));
    EXPECT_EQ(24, next_record_size) << strerror(ringfile.error());

    std::string buffer;
    buffer.resize(next_record_size);
    EXPECT_TRUE(ringfile.Read(const_cast<char *>(buffer.c_str()),
      next_record_size));
    EXPECT_EQ("012345678901234567890123", buffer);

    EXPECT_FALSE(ringfile.NextRecordSize(NULL));
  }
}


// This test checks that a write can work for the maximum size message but
// not for a larger one
TEST(RingfileTest, CannotWriteMessageAboveLimitStreaming) {
  std::string path = TempDir() + "/ring";

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Create(path, 50));

    // The maximum record size including header is 50 - 24 - 1 = 25 bytes
    // Which is a 1-byte header and a 24 byte message
    std::string message("0123456789012345678901234");  // 25 bytes
    ASSERT_FALSE(ringfile.StreamingWriteStart(message.size()));
    ringfile.Close();
  }

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Open(path, Ringfile::kAppend));
    std::string message = "012345678901234567890123";  // 24 bytes
    ASSERT_TRUE(ringfile.StreamingWriteStart(message.size()));
    ASSERT_TRUE(ringfile.StreamingWrite(message.c_str(), message.size()));
    ASSERT_TRUE(ringfile.StreamingWriteFinish());
    ringfile.Close();
  }

  EXPECT_EQ(std::string(
    "RING"  // magic number
    "\x00\x00\x00\x00"  // flags
    "\x00\x00\x00\x00\x00\x00\x00\x00"  // start offset
    "\x19\x00\x00\x00\x00\x00\x00\x00"  // end offset
    "\x18" "012345678901234567890123"  // record
    "\x00"  // unspecified
    , 50), GetFileContents(path));

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Open(path, Ringfile::kRead))
      << strerror(ringfile.error());

    std::string buffer;
    EXPECT_EQ(24, ringfile.StreamingReadStart());
    while (true) {
      char buffer_part[5];
      size_t buffer_part_size = ringfile.StreamingRead(buffer_part, 5);
      if (buffer_part_size == 0) {
        break;
      }
      ASSERT_NE(-1, buffer_part_size);
      buffer.append(std::string(buffer_part, buffer_part_size));
    }
    ringfile.StreamingReadFinish();
    EXPECT_EQ("012345678901234567890123", buffer);

    EXPECT_FALSE(ringfile.NextRecordSize(NULL));
  }
}

// This test checks that the write can work when the offset is at the end of
// the file.
TEST(RingfileTest, CanAppendEdge) {
  std::string path = TempDir() + "/ring";

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Create(path, 50));

    // The maximum record size including header is 50 - 24 - 1 = 25 bytes
    // Which is a 1-byte header and a 24 byte message
    std::string message("012345678901234567890123");
    ASSERT_TRUE(ringfile.Write(message.c_str(), message.size()));

    message = "abcd";
    ASSERT_TRUE(ringfile.Write(message.c_str(), message.size()));

    ringfile.Close();
  }

  EXPECT_EQ(std::string(
    "RING"  // magic number
    "\x00\x00\x00\x00"  // flags
    "\x19\x00\x00\x00\x00\x00\x00\x00"  // start offset
    "\x04\x00\x00\x00\x00\x00\x00\x00"  // end offset
    "abcd"  // record
    "345678901234567890123"  // old record
    "\x04"  // first part of header
    , 50), GetFileContents(path));

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Open(path, Ringfile::kRead))
      << strerror(ringfile.error());

    size_t next_record_size;
    EXPECT_TRUE(ringfile.NextRecordSize(&next_record_size));
    EXPECT_EQ(4, next_record_size) << strerror(ringfile.error());

    std::string buffer;
    buffer.resize(next_record_size);
    EXPECT_TRUE(ringfile.Read(const_cast<char *>(buffer.c_str()),
      next_record_size));
    EXPECT_EQ("abcd", buffer);

    EXPECT_FALSE(ringfile.NextRecordSize(NULL));
  }
}

TEST(RingfileTest, CanAppendEdgeStreaming) {
  std::string path = TempDir() + "/ring";

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Create(path, 50));

    // The maximum record size including header is 50 - 24 - 1 = 25 bytes
    // Which is a 1-byte header and a 24 byte message
    std::string message("012345678901234567890123");
    ASSERT_TRUE(ringfile.StreamingWriteStart(message.size()));
    ASSERT_TRUE(ringfile.StreamingWrite(message.c_str(), message.size()));
    ASSERT_TRUE(ringfile.StreamingWriteFinish())
      << strerror(ringfile.error());

    message = "abcd";
    ASSERT_TRUE(ringfile.StreamingWriteStart(message.size()));
    ASSERT_TRUE(ringfile.StreamingWrite(message.c_str(), message.size()));
    ASSERT_TRUE(ringfile.StreamingWriteFinish())
      << strerror(ringfile.error());

    ringfile.Close();
  }

  EXPECT_EQ(std::string(
    "RING"  // magic number
    "\x00\x00\x00\x00"  // flags
    "\x19\x00\x00\x00\x00\x00\x00\x00"  // start offset
    "\x04\x00\x00\x00\x00\x00\x00\x00"  // end offset
    "abcd"  // record
    "345678901234567890123"  // old record
    "\x04"  // first part of header
    , 50), GetFileContents(path));

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Open(path, Ringfile::kRead))
      << strerror(ringfile.error());

    size_t next_record_size;
    EXPECT_TRUE(ringfile.NextRecordSize(&next_record_size));
    EXPECT_EQ(4, next_record_size) << strerror(ringfile.error());

    std::string buffer;
    buffer.resize(4);
    EXPECT_EQ(4, ringfile.StreamingReadStart());
    size_t buffer_part_size = ringfile.StreamingRead(
      const_cast<char *>(buffer.c_str()), 5);
    EXPECT_EQ(4, buffer_part_size);
    EXPECT_EQ(0, ringfile.StreamingRead(NULL, 5));
    ringfile.StreamingReadFinish();

    EXPECT_EQ("abcd", buffer);

    next_record_size = ringfile.StreamingReadStart();
    EXPECT_EQ(-1, next_record_size);
  }
}
