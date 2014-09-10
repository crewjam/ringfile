#!/bin/bash
set -ex
if [ "$TRAVIS_OS_NAME" == "linux" ] ; then
  apt-get install -qq autoconf-archive
fi

if [ "$TRAVIS_OS_NAME" == "osx" ] ; then
  # brew ...
fi

./autogen.sh
./configure
make
make check
