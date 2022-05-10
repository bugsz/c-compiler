CURDIR=$(cd $(dirname $0); pwd)
cd $CURDIR/src/frontend
sh ./run_script.sh
sh ./configure
make

cd $CURDIR
! test -d build && mkdir build
cd build
cmake ..
make