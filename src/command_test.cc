// Copyright (c) 2014 Ross Kinder. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "command.h"

#include <gtest/gtest.h>
#include <sstream>


template<class Type, ptrdiff_t n>
ptrdiff_t arraysize(Type (&)[n]) { return n; }

TEST(CommandTest, CanParseArguments1) {
  char * argv[] = {"frob", "--help"};
  std::stringstream error_stream;

  Switches arguments;
  arguments.error_stream = &error_stream;

  EXPECT_EQ(true, arguments.Parse(arraysize(argv), argv));
  EXPECT_EQ("frob", arguments.program);
  EXPECT_EQ("usage: frob [-hvSas] [file]\n", error_stream.str());
}

TEST(CommandTest, CanParseArguments2) {
  char * argv[] = {"frob", "-v", "-s", "400m", "-a", "some_path"};
  std::stringstream error_stream;

  Switches arguments;
  arguments.error_stream = &error_stream;

  EXPECT_EQ(true, arguments.Parse(arraysize(argv), argv));
  EXPECT_EQ("frob", arguments.program);
  EXPECT_EQ("", error_stream.str());
  EXPECT_EQ(1, arguments.verbose);
  EXPECT_EQ(400 * 1024 * 1024, arguments.size);
  EXPECT_EQ(Switches::kModeAppend, arguments.mode);
  EXPECT_EQ("some_path", arguments.path);
}

TEST(CommandTest, CanParseArguments3) {
  char * argv[] = {"frob", "-v", "-s", "400m", "-S", "some_path"};
  std::stringstream error_stream;

  Switches arguments;
  arguments.error_stream = &error_stream;

  EXPECT_EQ(true, arguments.Parse(arraysize(argv), argv));
  EXPECT_EQ("frob", arguments.program);
  EXPECT_EQ("", error_stream.str());
  EXPECT_EQ(1, arguments.verbose);
  EXPECT_EQ(400 * 1024 * 1024, arguments.size);
  EXPECT_EQ(Switches::kModeStat, arguments.mode);
  EXPECT_EQ("some_path", arguments.path);
}

TEST(CommandTest, CanParseArguments4) {
  char * argv[] = {"frob", "some_path"};
  std::stringstream error_stream;

  Switches arguments;
  arguments.error_stream = &error_stream;

  EXPECT_EQ(true, arguments.Parse(arraysize(argv), argv));
  EXPECT_EQ("frob", arguments.program);
  EXPECT_EQ("", error_stream.str());
  EXPECT_EQ(0, arguments.verbose);
  EXPECT_EQ(-1, arguments.size);
  EXPECT_EQ(Switches::kModeRead, arguments.mode);
  EXPECT_EQ("some_path", arguments.path);
}

TEST(CommandTest, CanParseArguments5) {
  char * argv[] = {"frob", "--verbose", "--size", "400m", "--append",
    "some_path"};
  std::stringstream error_stream;

  Switches arguments;
  arguments.error_stream = &error_stream;

  EXPECT_EQ(true, arguments.Parse(arraysize(argv), argv));
  EXPECT_EQ("frob", arguments.program);
  EXPECT_EQ("", error_stream.str());
  EXPECT_EQ(1, arguments.verbose);
  EXPECT_EQ(400 * 1024 * 1024, arguments.size);
  EXPECT_EQ(Switches::kModeAppend, arguments.mode);
  EXPECT_EQ("some_path", arguments.path);
}

TEST(CommandTest, CanParseArguments6) {
  char * argv[] = {"frob", "--size", "frob", "--append", "some_path"};
  std::stringstream error_stream;

  Switches arguments;
  arguments.error_stream = &error_stream;

  EXPECT_EQ(false, arguments.Parse(arraysize(argv), argv));
  EXPECT_EQ("frob", arguments.program);
  EXPECT_EQ("frob: invalid size\n", error_stream.str());
}

TEST(CommandTest, CanParseArguments7) {
  char * argv[] = {"frob", "--size", "100q", "--append", "some_path"};
  std::stringstream error_stream;

  Switches arguments;
  arguments.error_stream = &error_stream;

  EXPECT_EQ(false, arguments.Parse(arraysize(argv), argv));
  EXPECT_EQ("frob", arguments.program);
  EXPECT_EQ("frob: invalid size\n", error_stream.str());
}

TEST(CommandTest, CanParseArguments8) {
  char * argv[] = {"frob", "--frob"};
  std::stringstream error_stream;

  Switches arguments;
  arguments.error_stream = &error_stream;

  EXPECT_EQ(false, arguments.Parse(arraysize(argv), argv));
  EXPECT_EQ("frob", arguments.program);
  EXPECT_EQ("frob: invalid option\n", error_stream.str());
}

TEST(CommandTest, CanParseArguments9) {
  char * argv[] = {"frob", "some_path", "some other path"};
  std::stringstream error_stream;

  Switches arguments;
  arguments.error_stream = &error_stream;

  EXPECT_EQ(false, arguments.Parse(arraysize(argv), argv));
  EXPECT_EQ("frob", arguments.program);
  EXPECT_EQ("frob: too many file arguments\n", error_stream.str());
}

TEST(CommandTest, CanParseArguments10) {
  char * argv[] = {"frob", "--verbose"};
  std::stringstream error_stream;

  Switches arguments;
  arguments.error_stream = &error_stream;

  EXPECT_EQ(false, arguments.Parse(arraysize(argv), argv));
  EXPECT_EQ("frob", arguments.program);
  EXPECT_EQ("frob: missing file argument\n", error_stream.str());
}
