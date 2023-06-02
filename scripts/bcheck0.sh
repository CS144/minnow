#!/bin/bash
g++ -g -c -I src src/byte_stream.cc -o bs.o
g++ -g -c -I src src/byte_stream_helpers.cc -o bsh.o
g++ -g -c -I src tests/check0test.cpp -o c0t.o
g++ -g -L. c0t.o bs.o bsh.o -o c0t.out
./c0t.out