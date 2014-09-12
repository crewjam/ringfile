// Copyright (c) 2014 Ross Kinder. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef TEST_UTIL_H_
#define TEST_UTIL_H_

#include <string>

// Register `path` to be deleted in an atexit() handler.
void RemotePathAtExit(const std::string & path);

// Return a new temporary directory which will be deleted when the program
// exits.
std::string TempDir();

// Return the contents of `path` as a string.
std::string GetFileContents(const std::string & path);

#endif  // TEST_UTIL_H_
