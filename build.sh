#!/bin/bash
set -x
rm -rf build/
mkdir build
cd build
#cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_TEST=ON
