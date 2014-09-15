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
    "\x11\x00\x00\x00\x00\x00\x00\x00"  // end offset
    "\x0d\x00\x00\x00"  // record length
    "Hello, World!", 41), GetFileContents(path).substr(0, 41));

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
    "\x11\x00\x00\x00\x00\x00\x00\x00"  // start offset
    "\x08\x00\x00\x00\x00\x00\x00\x00"  // end offset
    "ye, Bob!"  // end of the record
    "o, World!"  // leftover from hello world
    "\x0d\x00\x00\x00Goodb"  // beginning of message
    , 50), GetFileContents(path));

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
    EXPECT_EQ("Goodbye, Bob!", buffer);

    EXPECT_FALSE(ringfile.NextRecordSize(NULL));
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
    "\x08\x00\x00\x00\x00\x00\x00\x00"  // start offset
    "\x19\x00\x00\x00\x00\x00\x00\x00"  // end offset
    "FGHIJKLM"  // leftover from record 2
    "\x0d\x00\x00\x00nopqrstuvwxyz"  // record 3
    "E"  // trailing byte (??)
    , 50), GetFileContents(path));

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
    EXPECT_EQ("nopqrstuvwxyz", buffer);

    EXPECT_FALSE(ringfile.NextRecordSize(NULL));
  }
}

// This test checks that a write can work for the maximum size message but
// not for a larger one
TEST(RingfileTest, CannotWriteMessageAboveLimit) {
  std::string path = TempDir() + "/ring";

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Create(path, 50));

    
    // The maximum message size is 50 - 24 - 4 - 1= 21 bytes
    
    std::string message("0123456789012345678901");  // 22 bytes
    ASSERT_FALSE(ringfile.Write(message.c_str(), message.size()));

    message = "012345678901234567890";  // 21 bytes
    ASSERT_TRUE(ringfile.Write(message.c_str(), message.size()));
    ringfile.Close();
  }

  EXPECT_EQ(std::string(
    "RING"  // magic number
    "\x00\x00\x00\x00"  // flags
    "\x00\x00\x00\x00\x00\x00\x00\x00"  // start offset
    "\x19\x00\x00\x00\x00\x00\x00\x00"  // end offset
    "\x15\x00\x00\x00" "012345678901234567890"  // record
    "\x00"  // unspecified
    , 50), GetFileContents(path));

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Open(path, Ringfile::kRead))
      << strerror(ringfile.error());

    size_t next_record_size;
    EXPECT_TRUE(ringfile.NextRecordSize(&next_record_size));
    EXPECT_EQ(21, next_record_size) << strerror(ringfile.error());

    std::string buffer;
    buffer.resize(next_record_size);
    EXPECT_TRUE(ringfile.Read(const_cast<char *>(buffer.c_str()),
      next_record_size));
    EXPECT_EQ("012345678901234567890", buffer);

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

    // The maximum message size is 50 - 24 - 4 = 21 bytes
    std::string message("012345678901234567890");
    ASSERT_TRUE(ringfile.Write(message.c_str(), message.size()));

    message = "abcd";
    ASSERT_TRUE(ringfile.Write(message.c_str(), message.size()));

    ringfile.Close();
  }

  EXPECT_EQ(std::string(
    "RING"  // magic number
    "\x00\x00\x00\x00"  // flags
    "\x19\x00\x00\x00\x00\x00\x00\x00"  // start offset
    "\x07\x00\x00\x00\x00\x00\x00\x00"  // end offset
    "\0\0\0" // second part of record header
    "abcd" // record
    "345678901234567890" // old record
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


TEST(RingfileTest, CanReadAndWriteBasicsStreaming) {
  std::string path = TempDir() + "/ring";

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Create(path, 1024));

    ASSERT_EQ(true, ringfile.StreamingWriteStart());
    ASSERT_EQ(true, ringfile.StreamingWrite("Hello, ", 7));
    ASSERT_EQ(true, ringfile.StreamingWrite("World!", 6));
    ASSERT_EQ(true, ringfile.StreamingWriteFinish());
    ringfile.Close();
  }

  EXPECT_EQ(std::string(
    "RING"  // magic number
    "\x00\x00\x00\x00"  // flags
    "\x00\x00\x00\x00\x00\x00\x00\x00"  // start offset
    "\x11\x00\x00\x00\x00\x00\x00\x00"  // end offset
    "\x0d\x00\x00\x00"  // record length
    "Hello, World!", 41), GetFileContents(path).substr(0, 41));

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Open(path, Ringfile::kRead))
      << strerror(ringfile.error());

    EXPECT_EQ(13, ringfile.StreamingReadStart());
    
    char buffer[5];
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

TEST(RingfileTest, CircularWriteStreaming) {
  std::string path = TempDir() + "/ring";

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Create(path, 50));
    
    ASSERT_EQ(true, ringfile.StreamingWriteStart());
    ASSERT_EQ(true, ringfile.StreamingWrite("Hello, ", 7));
    ASSERT_EQ(true, ringfile.StreamingWrite("World!", 6));
    ASSERT_EQ(true, ringfile.StreamingWriteFinish());
    ringfile.Close();
  }

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Open(path, Ringfile::kAppend));
    
    std::string message("Goodbye, Bob!");
    ASSERT_EQ(true, ringfile.StreamingWriteStart());
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
    "\x0d\x00\x00\x00Goodb"  // beginning of message
    , 50), GetFileContents(path));

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Open(path, Ringfile::kRead))
      << strerror(ringfile.error());

    EXPECT_EQ(13, ringfile.StreamingReadStart());
    
    char buffer[5];
    EXPECT_EQ(5, ringfile.StreamingRead(&buffer, 5));
    EXPECT_STREQ("Goodb", buffer);
    
    EXPECT_EQ(5, ringfile.StreamingRead(&buffer, 5));
    EXPECT_STREQ("ye, B", buffer);
    
    buffer[3] = 0;
    EXPECT_EQ(3, ringfile.StreamingRead(&buffer, 5));
    EXPECT_STREQ("ob!", buffer);
    
    EXPECT_EQ(true, ringfile.StreamingReadFinish());
    
    EXPECT_EQ(-1, ringfile.StreamingReadStart());
    EXPECT_FALSE(ringfile.NextRecordSize(NULL));
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
    "\x08\x00\x00\x00\x00\x00\x00\x00"  // start offset
    "\x19\x00\x00\x00\x00\x00\x00\x00"  // end offset
    "FGHIJKLM"  // leftover from record 2
    "\x0d\x00\x00\x00nopqrstuvwxyz"  // record 3
    "E"  // trailing byte (??)
    , 50), GetFileContents(path));

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Open(path, Ringfile::kRead))
      << strerror(ringfile.error());

    std::string buffer;
    EXPECT_EQ(13, ringfile.StreamingReadStart());
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
    EXPECT_EQ("nopqrstuvwxyz", buffer);

    EXPECT_EQ(-1, ringfile.StreamingReadStart());
  }
}



// This test checks that a write can work for the maximum size message but
// not for a larger one
TEST(RingfileTest, CannotWriteMessageAboveLimitStreaming) {
  std::string path = TempDir() + "/ring";

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Create(path, 50));

    // The maximum message size is 50 - 24 - 4 - 1 = 21 bytes
    std::string message("0123456789012345678901");  // 22 bytes
    ASSERT_TRUE(ringfile.StreamingWriteStart());
    ASSERT_FALSE(ringfile.StreamingWrite(message.c_str(), message.size()));
    ringfile.Close();
  }
  
  
  EXPECT_EQ(std::string(
    "RING"  // magic number
    "\x00\x00\x00\x00"  // flags
    "\x00\x00\x00\x00\x00\x00\x00\x00"  // start offset
    "\x00\x00\x00\x00\x00\x00\x00\x00"  // end offset
    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
    , 50), GetFileContents(path));
  
  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Open(path, Ringfile::kAppend));
    std::string message = "012345678901234567890";  // 21 bytes
    ASSERT_TRUE(ringfile.StreamingWriteStart());
    ASSERT_TRUE(ringfile.StreamingWrite(message.c_str(), message.size()));
    ASSERT_TRUE(ringfile.StreamingWriteFinish());
    ringfile.Close();
  }

  EXPECT_EQ(std::string(
    "RING"  // magic number
    "\x00\x00\x00\x00"  // flags
    "\x00\x00\x00\x00\x00\x00\x00\x00"  // start offset
    "\x19\x00\x00\x00\x00\x00\x00\x00"  // end offset
    "\x15\x00\x00\x00" "012345678901234567890"  // record
    "\x00"  // unspecified
    , 50), GetFileContents(path));

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Open(path, Ringfile::kRead))
      << strerror(ringfile.error());

    std::string buffer;
    EXPECT_EQ(21, ringfile.StreamingReadStart());
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
    EXPECT_EQ("012345678901234567890", buffer);

    EXPECT_FALSE(ringfile.NextRecordSize(NULL));
  }
}

TEST(RingfileTest, CanAppendEdgeStreaming) {
  std::string path = TempDir() + "/ring";

  {
    Ringfile ringfile;
    ASSERT_TRUE(ringfile.Create(path, 50));

    // The maximum message size is 50 - 24 - 4 = 21 bytes
    std::string message("012345678901234567890");
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
    "\x19\x00\x00\x00\x00\x00\x00\x00"  // start offset
    "\x07\x00\x00\x00\x00\x00\x00\x00"  // end offset
    "\0\0\0" // second part of record header
    "abcd" // record
    "345678901234567890" // old record
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
    size_t buffer_part_size = ringfile.StreamingRead(const_cast<char *>(buffer.c_str()), 5);
    EXPECT_EQ(4, buffer_part_size);
    EXPECT_EQ(0, ringfile.StreamingRead(NULL, 5));
    ringfile.StreamingReadFinish();
    
    EXPECT_EQ("abcd", buffer);

    next_record_size = ringfile.StreamingReadStart();
    EXPECT_EQ(-1, next_record_size);
  }
}
