CURDIR=$(cd $(dirname $0); pwd)

test -e serializer && rm -f serializer
test -e llvm_wrapper && rm -f llvm_wrapper
sh $CURDIR/../serialization/compile.sh $1
cp $CURDIR/../serialization/serializer $CURDIR/bin/serializer
test -d $CURDIR/bin/build && rm -rf $CURDIR/bin/build
mkdir $CURDIR/bin/build
cd $CURDIR/bin/build
cmake -DIRONLY=ON $CURDIR/../../.. && make
test -e "zjucc_backend" && echo "Backend build success"
cp zjucc_backend $CURDIR/bin/llvm_wrapper