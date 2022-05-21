#!/bin/bash
CURDIR=$(cd $(dirname $0); pwd)
echo "Compile serializer in " $CURDIR
sh $CURDIR/clean.sh
if [ "x$1" != "x" ]; then
    echo "Also recompile libzjucc"
    cd $CURDIR/../../frontend
    sh clean.sh
    sh run-script.sh
    ./configure && make
    test -e "libzjucc.a" &&  echo "Lib zjucc build success"
else
    cd $CURDIR/../../frontend
    test -e "Makefile" && make
fi
g++ $CURDIR/*.cpp -I$CURDIR/../../frontend -L$CURDIR/../../frontend -lzjucc -lfmt -std=c++17 -o  $CURDIR/serializer && echo "Serializer build success"
