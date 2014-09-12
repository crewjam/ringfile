# Copyright (c) 2014 Ross Kinder. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import atexit
import shutil
import tempfile
import unittest

from ringfile import Ringfile

def TempDir():
  path = tempfile.mkdtemp()
  atexit.register(shutil.rmtree, path)
  return path

class RingFileTest(unittest.TestCase):
  def testCannotOpenBogusPath(self):
    path = TempDir() + "/does not exist/ring"
    with self.assertRaises(IOError):
      Ringfile.Create(path, 1000)

    with self.assertRaises(IOError):
      Ringfile(path, "r")

    with self.assertRaises(IOError):
      Ringfile(path, "a")

    path = TempDir() + "/ring"
    with self.assertRaises(IOError):
      Ringfile(path, "r")
    with self.assertRaises(IOError):
      Ringfile(path, "a")

  def testCanReadAndWriteBasics(self):
    path = TempDir() + "/ring"

    ringfile = Ringfile.Create(path, 1024)
    ringfile.Write("Hello, World!")
    ringfile.Close()

    ringfile = Ringfile(path, "a")
    ringfile.Write("Goodbye, World!")
    ringfile.Close()

    ringfile = Ringfile(path, "r")
    self.assertEqual("Hello, World!", ringfile.Read())
    self.assertEqual("Goodbye, World!", ringfile.Read())
    self.assertEqual(None, ringfile.Read())


if __name__ == "__main__":
  unittest.main()