# Copyright (c) 2014 Ross Kinder. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import os
import shutil
from os.path import basename, join as pathjoin

from setuptools import setup, Extension


srcdir = "."
sources = [
  "module.cc",
  "../src/ringfile.cc",
  "../src/varint.cc",
]


# hacks to support VPATH builds in setup.py (required for make distcheck)
if os.environ.get("VPATH"):
  srcdir = os.environ["VPATH"]
  for path in sources:
    print "cp", pathjoin(os.environ["VPATH"], path), basename(path)
    shutil.copyfile(pathjoin(os.environ["VPATH"], path),
      basename(path))
  sources = map(basename, sources)


setup(
  name="ringfile",
  description="A tool and library for writing and reading fixed size circular log files.",
  version="0.9",
  license="BSD",
  url="https://github.com/crewjam/ringfile",
  author="Ross Kinder",
  author_email="ross+czNyZXBv@kndr.org",
  ext_modules=[Extension('ringfile', sources,
    include_dirs=[
      srcdir + "/../include",
      srcdir + "/../src",
      srcdir + "/.",
    ],
  )],
  test_suite='ringfile_test',
)
