#!/bin/bash
set -x

CFLAGS="-g -I./zstd/lib zstd/lib/libzstd.a src/dictionary.s"
g++ src/server.cpp -o bin/server.exe $CFLAGS &
g++ src/client.cpp -o bin/client.exe $CFLAGS &
wait
