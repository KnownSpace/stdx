#!/bin/bash
git clone https://github.com/jemalloc/jemalloc
cd jemalloc
./autogen.sh
make
make install