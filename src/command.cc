// Copyright (c) 2014 Ross Kinder. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "command.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>

Switches::Switches()
  : error_stream(&std::cerr),
    mode(kModeUnspecified),
    verbose(0),
    size(-1) {
}

bool Switches::Parse(int argc, char ** argv) {
  optind = 1;  // reset global state
  int option_index = 0;
  program = argv[0];

  while (true) {
    static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"verbose", no_argument, 0, 'v'},
      {"size", required_argument, 0, 's'},
      {"stat", no_argument, 0, 'S'},
      {"append", no_argument, 0, 'a'},
      {0, 0, 0, 0}
    };

    int option = getopt_long(argc, argv, "hvs:Sa", long_options, &option_index);

    if (option == -1) {
      break;
    }

    if (option == 'h') {
        *error_stream << "usage: " << program << " [-hvSas] [file]\n";
      return true;
    }

    if (option == 'v') {
      verbose = 1;
      continue;
    }

    if (option == kModeStat || option == kModeRead || option == kModeAppend) {
      if (mode != kModeUnspecified) {
        *error_stream << program << ": cannot specify more than one mode "
          "option\n";
        return false;
      }
      mode = option;
      continue;
    }

    if (option == 's') {
      errno = 0;
      char * units;
      size = strtol(optarg, &units, 10);
      if (errno != 0 || size <= 0) {
        *error_stream << program << ": invalid size\n";
        return false;
      }

      if (*units == 0 || strcmp(units, "b") == 0 || strcmp(units, "B") == 0) {
        // size in bytes, nop
      } else if (strcmp(units, "k") == 0 || strcmp(units, "K") == 0) {
        size *= 1024;  // size in kbytes
      } else if (strcmp(units, "m") == 0 || strcmp(units, "M") == 0) {
        size *= 1024 * 1024;  // size in mb
      } else if (strcmp(units, "g") == 0 || strcmp(units, "G") == 0) {
        size *= 1024 * 1024 * 1024;  // size in gb
      } else {
        *error_stream << program << ": invalid size\n";
        return false;
      }
      continue;
    }

    *error_stream << program << ": invalid option\n";
    return false;
  }

  if (mode == kModeUnspecified) {
    mode = kModeRead;
  }

  // get the file path
  if (optind + 1 > argc) {
    *error_stream << program << ": missing file argument\n";
    return false;
  }
  if (optind + 1 < argc) {
    *error_stream << program << ": too many file arguments\n";
    return false;
  }
  path = argv[optind];
  return true;
}


int Main(int argc, char **argv) {
  Switches arguments;
  if (!arguments.Parse(argc, argv)) {
    return 1;
  }

  std::cout << "mode=" << arguments.mode << " verbose=" << arguments.verbose
    << " size=" << arguments.size << " path=" << arguments.path
    << " program=" << arguments.program << "\n";

  return 0;
}
