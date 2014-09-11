// Copyright (c) 2014 Ross Kinder. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "ringfile_internal.h"

#include <gtest/gtest.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

std::string TempDir() {
  char * temp_path = getenv("TEMP");
  if (!temp_path) {
    temp_path = getenv("TMP");
  }
  if (!temp_path) {
    temp_path = getenv("TMPDIR");
  }
  if (!temp_path) {
    temp_path = ".";
  }

  std::string path(temp_path);
  path += "/ringfile_test_XXXXXX";
  if (mkdtemp(const_cast<char *>(path.c_str())) == NULL) {
    return "";
  }
  return path;
}

std::string GetFileContents(const std::string & path) {
  struct stat buf;
  if (stat(path.c_str(), &buf) == -1) {
    return "";
  }
  int fd = open(path.c_str(), O_RDONLY);
  if (fd == -1) {
    return "";
  }

  std::string contents;
  contents.resize(buf.st_size);
  if (read(fd, const_cast<char *>(contents.c_str()), contents.size()) !=
      contents.size()) {
    return "";
  }

  close(fd);
  return contents;
}

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
