// Copyright (c) 2014 Ross Kinder. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef COMMAND_H_
#define COMMAND_H_

#include <iostream>
#include <string>

class Switches {
 public:
  enum {
    kModeUnspecified = 0,
    kModeRead='r',
    kModeStat='S',
    kModeAppend='a'
  };

  Switches();
  bool Parse(int argc, char ** argv);

  std::ostream * error_stream;
  int mode;
  int verbose;
  long size;
  std::string path;
  std::string program;
};

int Main(int argc, char **argv);

#endif  // COMMAND_H_
