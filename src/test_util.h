// Copyright (c) 2014 Ross Kinder. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef TEST_UTIL_H_
#define TEST_UTIL_H_

#include <string>

std::string TempDir();
std::string GetFileContents(const std::string & path);

#endif  // TEST_UTIL_H_
