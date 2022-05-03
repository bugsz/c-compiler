#!/bin/bash
if [ "x$1" != "x" ]; then
    cd ../../frontend
    sh clean.sh
    ./configure
    make
    cd ../visualization/serialization
fi
g++ *.cpp -Iinclude -I../../frontend -Llib -L../../frontend -lzjucc -lfmt -std=c++17 -o serializer