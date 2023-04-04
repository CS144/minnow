#!/bin/sh
exec make -j`nproc` "$@"
