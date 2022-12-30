#!/bin/bash
set -ex
CFLAGS="-static -Ofast -I./zstd/lib zstd/lib/libzstd.a src/dictionary.s"
g++ src/server.cpp -o bin/server.static.exe $CFLAGS &
g++ src/client.cpp -o bin/client.static.exe $CFLAGS &
python submit.py bin/server.static.exe bin/client.static.exe ${TOKEN}
