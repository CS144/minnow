#!/bin/bash
cd build
make
cd ..
cmake --build build --target check1