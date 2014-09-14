#!/usr/bin/env python
# Copyright (c) 2014 Ross Kinder. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import atexit
import shutil
import tempfile
import unittest

from ringfile import Ringfile, MODE_READ, MODE_APPEND

def TempDir():
  path = tempfile.mkdtemp()
  atexit.register(shutil.rmtree, path)
  return path

class RingFileTest(unittest.TestCase):
  def testCannotOpenBogusPath(self):
    path = TempDir() + "/does not exist/ring"
    with self.assertRaises(IOError):
      Ringfile.create(path, 1000)

    with self.assertRaises(IOError):
      Ringfile(path, MODE_READ)

    with self.assertRaises(IOError):
      Ringfile(path, MODE_APPEND)

    path = TempDir() + "/ring"
    with self.assertRaises(IOError):
      Ringfile(path, MODE_READ)
    with self.assertRaises(IOError):
      Ringfile(path, MODE_APPEND)

  def testCanReadAndWriteBasics(self):
    path = TempDir() + "/ring"

    ringfile = Ringfile.create(path, 1024)
    ringfile.write("Hello, World!")
    ringfile.close()

    ringfile = Ringfile(path, MODE_APPEND)
    ringfile.write("Goodbye, World!")
    ringfile.close()

    ringfile = Ringfile(path, MODE_READ)
    self.assertEqual("Hello, World!", ringfile.read())
    self.assertEqual("Goodbye, World!", ringfile.read())
    self.assertEqual(None, ringfile.read())

  def testDocsExist(self):
    self.assertTrue(Ringfile.__doc__)
    for name in dir(Ringfile):
      if name.startswith("_"):
        continue
      self.assertTrue(getattr(Ringfile, name).__doc__, name)

if __name__ == "__main__":
  unittest.main()
