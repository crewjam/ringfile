// Copyright (c) 2014 Ross Kinder. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "test_util.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

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
