// Copyright (c) 2014 Ross Kinder. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef COMMAND_H_
#define COMMAND_H_

#include <iostream>
#include <string>
#include <vector>

class Command {
 public:
  enum {
    kModeUnspecified = 0,
    kModeRead='r',
    kModeStat='S',
    kModeAppend='a'
  };

  Command();

  int Main(int argc, char ** argv);
  bool Parse(int argc, char ** argv);
  bool Read();
  bool Write();
  bool Stat();

  std::istream * stdin;
  std::ostream * stdout;
  std::ostream * stderr;
  int mode;
  int verbose;
  long size;
  std::string path;
  std::string program;
};


#endif  // COMMAND_H_
