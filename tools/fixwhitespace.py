#!/usr/bin/env python

import argparse
import re
import sys

def FixWhitespace(path):
  lines = file(path, "rb").readlines()
  should_rewrite = False 
  for i, line in enumerate(lines):
    trailing_whitespace, = re.search("(\s*)$", line).groups()
    if trailing_whitespace == "\n":
      continue
    print "%s(%d): incorrect line ending: %r" % (path, i, trailing_whitespace)
    line = re.sub("(\s*)$", "\n", line)
    lines[i] = line
    should_rewrite = True
  file(path, "wb").write("".join(lines))    


def Main(args=sys.argv[1:]):
  parser = argparse.ArgumentParser()
  parser.add_argument("path", nargs="+")
  options = parser.parse_args(args)

  for path in options.path:
    FixWhitespace(path)

if __name__ == "__main__":
  Main()
