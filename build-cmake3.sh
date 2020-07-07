#!/bin/bash
mkdir build
cd build
cmake3 .. -DCMAKE_BUILD_TYPE=Release
make