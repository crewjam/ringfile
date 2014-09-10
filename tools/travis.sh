#!/bin/bash
set -ex
apt-get install -qq autoconf-archive
./autogen.sh
./configure
make
make check
