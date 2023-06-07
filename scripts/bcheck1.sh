#!/bin/bash
g++ -g -c -I src src/byte_stream.cc -o bs.o
g++ -g -c -I src src/byte_stream_helpers.cc -o bsh.o
g++ -g -c -I src src/reassembler.cc -o ra.o
g++ -g -c -I src tests/check1test.cpp -o c1t.o
g++ -g    -L .   c1t.o bs.o bsh.o ra.o -o c1t.out
gdb ./c1t.out