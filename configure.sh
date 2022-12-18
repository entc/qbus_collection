#!/bin/bash

# create config folders
mkdir etc
mkdir etc/base

# setup default config for the python example module
echo "{\"b\":\"socket:127.0.0.1:55530\"}" >> etc/base/python.json

# do the build
mkdir build
cd build
rm CMakeCache.txt
cmake .. -Wno-dev


