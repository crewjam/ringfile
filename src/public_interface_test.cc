// Copyright (c) 2014 Ross Kinder. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <errno.h>
#include <gtest/gtest.h>

#include <ringfile.h>
#include "test_util.h"

TEST(PublicInterfaceTest, CannotOpenBogusPath) {
  std::string path = TempDir() + "/does not exist/ring";
  EXPECT_EQ(NULL, ringfile_create(path.c_str(), 1000));
  EXPECT_EQ(NULL, ringfile_open(path.c_str(), "r"));
  EXPECT_EQ(NULL, ringfile_open(path.c_str(), "a"));

  path = TempDir() + "/ring";
  EXPECT_EQ(NULL, ringfile_open(path.c_str(), "r"));
  EXPECT_EQ(NULL, ringfile_open(path.c_str(), "a"));
}

TEST(PublicInterfaceTest, CanReadAndWriteBasics) {
  std::string path = TempDir() + "/ring";

  {
    RINGFILE * ringfile = ringfile_create(path.c_str(), 1024);
    ASSERT_TRUE(NULL != ringfile);

    std::string message("Hello, World!");
    EXPECT_EQ(message.size(),
      ringfile_write(message.c_str(), message.size(), ringfile));

    ringfile_close(ringfile);
  }

  {
    RINGFILE * ringfile = ringfile_open(path.c_str(), "a");
    ASSERT_TRUE(NULL != ringfile);

    std::string message("Goodbye, World!");
    EXPECT_EQ(message.size(),
      ringfile_write(message.c_str(), message.size(), ringfile));

    ringfile_close(ringfile);
  }

  {
    RINGFILE * ringfile = ringfile_open(path.c_str(), "r");
    ASSERT_TRUE(NULL != ringfile);

    char buffer[100];
    size_t size = ringfile_next_record_size(ringfile);
    EXPECT_EQ(13, size);

    // Too small read
    EXPECT_EQ(-1, ringfile_read(buffer, 10, ringfile));

    EXPECT_EQ(13, ringfile_read(buffer, 100, ringfile));
    EXPECT_EQ("Hello, World!", std::string(buffer, size));

    size = ringfile_next_record_size(ringfile);
    EXPECT_EQ(15, ringfile_read(buffer, 100, ringfile));
    EXPECT_EQ("Goodbye, World!", std::string(buffer, size));

    size = ringfile_next_record_size(ringfile);
    EXPECT_EQ(-1, size);

    ringfile_close(ringfile);
  }
}

