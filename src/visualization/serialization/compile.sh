#!/bin/bash
CURDIR=$(cd $(dirname $0); pwd)
echo "Compile serializer in " $CURDIR
sh $CURDIR/clean.sh
if [ "x$1" != "x" ]; then
    echo "Also recompile libzjucc"
    cd $CURDIR/../../frontend
    sh clean.sh
    ./configure && make
    test -e "libzjucc.a" &&  echo "Lib zjucc build success"
fi
g++ $CURDIR/*.cpp -Iinclude -I$CURDIR/../../frontend -L$CURDIR/lib -L$CURDIR/../../frontend -lzjucc -lfmt -std=c++17 -o  $CURDIR/serializer && echo "Serializer build success"
