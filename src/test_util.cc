// Copyright (c) 2014 Ross Kinder. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "test_util.h"

#define _XOPEN_SOURCE 500

#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>

namespace {

static std::vector<std::string> cleanup_paths;

int RecursiveDeleteCallback(const char *fpath, const struct stat *sb,
                   int typeflag, struct FTW *ftwbuf) {
  if (typeflag == FTW_DP) {
    rmdir(fpath);
  } else if (typeflag == FTW_F) {
    unlink(fpath);
  }
  return 0;
}

void RecursiveDelete(const std::string & path) {
  nftw(path.c_str(), &RecursiveDeleteCallback, 20, FTW_DEPTH|FTW_PHYS);
}

void DoTempFileCleanup() {
  for (std::vector<std::string>::const_iterator path = cleanup_paths.begin();
      path != cleanup_paths.end(); ++path) {
    RecursiveDelete(*path);
  }
}

}  // anonymous namespace

void RemotePathAtExit(const std::string & path) {
  if (cleanup_paths.empty()) {
    atexit(&DoTempFileCleanup);
  }
  cleanup_paths.push_back(path);
}

std::string TempDir() {
  char * temp_path = getenv("TEMP");
  if (!temp_path) {
    temp_path = getenv("TMP");
  }
  if (!temp_path) {
    temp_path = getenv("TMPDIR");
  }
  if (!temp_path) {
    temp_path = "/tmp";
  }

  std::string path(temp_path);
  path += "/ringfile_test_XXXXXX";
  if (mkdtemp(const_cast<char *>(path.c_str())) == NULL) {
    return "";
  }
  RemotePathAtExit(path);
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
