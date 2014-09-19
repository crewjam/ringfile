// Copyright (c) 2014 Ross Kinder. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "command.h"

#include <gtest/gtest.h>

#include <sstream>

#include "test_util.h"

template<class Type, ptrdiff_t n>
ptrdiff_t arraysize(Type (&)[n]) { return n; }

TEST(CommandTest, CanParseArguments1) {
  char * argv[] = {"frob", "--help"};
  std::stringstream stderr;

  Command command;
  command.stderr = &stderr;

  EXPECT_EQ(true, command.Parse(arraysize(argv), argv));
  EXPECT_EQ("frob", command.program);
  EXPECT_EQ("usage: frob [-hvSas] [file]\n", stderr.str());
}

TEST(CommandTest, CanParseArguments2) {
  char * argv[] = {"frob", "-v", "-s", "400m", "-a", "some_path"};
  std::stringstream stderr;

  Command command;
  command.stderr = &stderr;

  EXPECT_EQ(true, command.Parse(arraysize(argv), argv));
  EXPECT_EQ("frob", command.program);
  EXPECT_EQ("", stderr.str());
  EXPECT_EQ(1, command.verbose);
  EXPECT_EQ(400 * 1024 * 1024, command.size);
  EXPECT_EQ(Command::kModeAppend, command.mode);
  EXPECT_EQ("some_path", command.path);
}

TEST(CommandTest, CanParseArguments3) {
  char * argv[] = {"frob", "-v", "-s", "400m", "-S", "some_path"};
  std::stringstream stderr;

  Command command;
  command.stderr = &stderr;

  EXPECT_EQ(true, command.Parse(arraysize(argv), argv));
  EXPECT_EQ("frob", command.program);
  EXPECT_EQ("", stderr.str());
  EXPECT_EQ(1, command.verbose);
  EXPECT_EQ(400 * 1024 * 1024, command.size);
  EXPECT_EQ(Command::kModeStat, command.mode);
  EXPECT_EQ("some_path", command.path);
}

TEST(CommandTest, CanParseArguments4) {
  char * argv[] = {"frob", "some_path"};
  std::stringstream stderr;

  Command command;
  command.stderr = &stderr;

  EXPECT_EQ(true, command.Parse(arraysize(argv), argv));
  EXPECT_EQ("frob", command.program);
  EXPECT_EQ("", stderr.str());
  EXPECT_EQ(0, command.verbose);
  EXPECT_EQ(-1, command.size);
  EXPECT_EQ(Command::kModeRead, command.mode);
  EXPECT_EQ("some_path", command.path);
}

TEST(CommandTest, CanParseArguments5) {
  char * argv[] = {"frob", "--verbose", "--size", "400m", "--append",
    "some_path"};
  std::stringstream stderr;

  Command command;
  command.stderr = &stderr;

  EXPECT_EQ(true, command.Parse(arraysize(argv), argv));
  EXPECT_EQ("frob", command.program);
  EXPECT_EQ("", stderr.str());
  EXPECT_EQ(1, command.verbose);
  EXPECT_EQ(400 * 1024 * 1024, command.size);
  EXPECT_EQ(Command::kModeAppend, command.mode);
  EXPECT_EQ("some_path", command.path);
}

TEST(CommandTest, CanParseArguments6) {
  char * argv[] = {"frob", "--size", "frob", "--append", "some_path"};
  std::stringstream stderr;

  Command command;
  command.stderr = &stderr;

  EXPECT_EQ(false, command.Parse(arraysize(argv), argv));
  EXPECT_EQ("frob", command.program);
  EXPECT_EQ("frob: invalid size\n", stderr.str());
}

TEST(CommandTest, CanParseArguments7) {
  char * argv[] = {"frob", "--size", "100q", "--append", "some_path"};
  std::stringstream stderr;

  Command command;
  command.stderr = &stderr;

  EXPECT_EQ(false, command.Parse(arraysize(argv), argv));
  EXPECT_EQ("frob", command.program);
  EXPECT_EQ("frob: invalid size\n", stderr.str());
}

TEST(CommandTest, CanParseArguments8) {
  char * argv[] = {"frob", "--frob"};
  std::stringstream stderr;

  Command command;
  command.stderr = &stderr;

  EXPECT_EQ(false, command.Parse(arraysize(argv), argv));
  EXPECT_EQ("frob", command.program);
  EXPECT_EQ("frob: invalid option\n", stderr.str());
}

TEST(CommandTest, CanParseArguments9) {
  char * argv[] = {"frob", "some_path", "some other path"};
  std::stringstream stderr;

  Command command;
  command.stderr = &stderr;

  EXPECT_EQ(false, command.Parse(arraysize(argv), argv));
  EXPECT_EQ("frob", command.program);
  EXPECT_EQ("frob: too many file arguments\n", stderr.str());
}

TEST(CommandTest, CanParseArguments10) {
  char * argv[] = {"frob", "--verbose"};
  std::stringstream stderr;

  Command command;
  command.stderr = &stderr;

  EXPECT_EQ(false, command.Parse(arraysize(argv), argv));
  EXPECT_EQ("frob", command.program);
  EXPECT_EQ("frob: missing file argument\n", stderr.str());
}

TEST(CommandTest, CanAppendReadAndWrite) {
  std::string path = TempDir() + "/ring";

  {
    char * argv[] = {"frob", NULL, "--append", "--size", "50"};
    argv[1] = const_cast<char *>(path.c_str());

    std::stringstream stdin;
    stdin.str("Hello, World!");

    Command command;
    command.stdin = &stdin;

    EXPECT_EQ(0, command.Main(arraysize(argv), argv));
  }

  {
    char * argv[] = {"frob", NULL, "--stat"};
    argv[1] = const_cast<char *>(path.c_str());
    std::stringstream stdout;

    Command command;
    command.stdout = &stdout;

    EXPECT_EQ(0, command.Main(arraysize(argv), argv));

    std::string expected_output = "File: ";
    expected_output += path;
    expected_output += "\nSize: 26 bytes\nUsed: 14 bytes\nFree: 12 bytes\n";

    EXPECT_EQ(expected_output, stdout.str());
  }

  {
    char * argv[] = {"frob", NULL, "--append", "--size", "50"};
    argv[1] = const_cast<char *>(path.c_str());

    std::stringstream stdin;
    stdin.str("Goodbye, World!");

    Command command;
    command.stdin = &stdin;

    EXPECT_EQ(0, command.Main(arraysize(argv), argv));
  }

  {
    char * argv[] = {"frob", NULL};
    argv[1] = const_cast<char *>(path.c_str());
    std::stringstream stdout;

    Command command;
    command.stdout = &stdout;

    EXPECT_EQ(0, command.Main(arraysize(argv), argv));
    EXPECT_EQ("Goodbye, World!\n", stdout.str());
  }
}
