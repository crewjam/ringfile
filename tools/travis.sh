#!/bin/bash
set -ex
sudo apt-get install -qq autoconf-archive
./autogen.sh
./configure
make
make check
