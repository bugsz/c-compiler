#!/bin/sh
CURDIR=$(cd $(dirname $0); pwd)
cd $CURDIR/src/frontend
make -j8

cd $CURDIR
! test -d build && mkdir build
cd build
cmake ..
make -j8
cd $CURDIR/build