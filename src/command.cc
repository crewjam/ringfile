// Copyright (c) 2014 Ross Kinder. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "command.h"

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ringfile_internal.h"

Command::Command()
  : stdin(&std::cin),
    stdout(&std::cout),
    stderr(&std::cerr),
    mode(kModeUnspecified),
    verbose(0),
    size(-1) {
}

bool Command::Parse(int argc, char ** argv) {
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
      *stderr << "usage: " << program << " [-hvSas] [file]\n";
      return true;
    }

    if (option == 'v') {
      verbose = 1;
      continue;
    }

    if (option == kModeStat || option == kModeRead || option == kModeAppend) {
      if (mode != kModeUnspecified) {
        *stderr << program << ": cannot specify more than one mode "
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
        *stderr << program << ": invalid size\n";
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
        *stderr << program << ": invalid size\n";
        return false;
      }
      continue;
    }

    *stderr << program << ": invalid option\n";
    return false;
  }

  if (mode == kModeUnspecified) {
    mode = kModeRead;
  }

  // get the file path
  if (optind + 1 > argc) {
    *stderr << program << ": missing file argument\n";
    return false;
  }
  if (optind + 1 < argc) {
    *stderr << program << ": too many file arguments\n";
    return false;
  }
  path = argv[optind];
  return true;
}

bool Command::Read() {
  Ringfile ring_file;
  if (!ring_file.Open(path, Ringfile::kRead)) {
    *stderr << path << ": " << strerror(ring_file.error()) << "\n";
    return false;
  }

  while (true) {
    size_t size;
    if (!ring_file.NextRecordSize(&size)) {
      break;
    }

    std::string record;
    record.resize(size);
    if (!ring_file.Read(const_cast<char *>(record.c_str()), record.size())) {
      *stderr << path << ": " << strerror(ring_file.error()) << "\n";
      return false;
    }
    *stdout << record << std::endl;
  }
  return true;
}

bool Command::Write() {
  Ringfile ring_file;

  if (!ring_file.Open(path, Ringfile::kAppend)) {
    if (ring_file.error() != ENOENT) {
      *stderr << path << ": cannot open: " << strerror(ring_file.error())
        << "\n";
      return false;
    }
    if (size == -1) {
      *stderr << path << ": does not exist and --size was not specified\n";
      return false;
    }

    if (!ring_file.Create(path, size)) {
      *stderr << path << ": cannot create: " << strerror(ring_file.error())
        << "\n";
      return false;
    }
  }

  // Treat each line as a record
  std::string line;
  while (std::getline(*stdin, line)) {
    if (!ring_file.Write(line.c_str(), line.size())) {
      *stderr << path << ": writing: " << strerror(ring_file.error()) << "\n";
      return false;
    }
  }

  return true;
}

bool Command::Stat() {
  Ringfile ring_file;
  if (!ring_file.Open(path, Ringfile::kRead)) {
    *stderr << path << ": " << strerror(ring_file.error()) << "\n";
    return false;
  }

  *stdout << "File: " << path << "\n";
  *stdout << "Size: " << ring_file.bytes_max() << " bytes\n";
  *stdout << "Used: " << ring_file.bytes_used() << " bytes\n";
  *stdout << "Free: " << ring_file.bytes_available() << " bytes\n";
  return true;
}

int Command::Main(int argc, char ** argv) {
  if (!Parse(argc, argv)) {
    return 1;
  }

  bool ok;
  switch (mode) {
    case kModeRead:
      ok = Read();
      break;
    case kModeAppend:
      ok = Write();
      break;
    case kModeStat:
      ok = Stat();
      break;
  }
  return ok ? 0 : 1;
}
