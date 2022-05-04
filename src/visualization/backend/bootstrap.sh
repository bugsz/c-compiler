filename=zju.c-compiler.backend
killall ${filename}
RED_BOLD=$(tput setaf 1;tput bold)
NORMAL=$(tput sgr0)
CURDIR=$(cd $(dirname $0); pwd)

echo "${RED_BOLD}Build Web Backend Binary${NORMAL}"
    test -d $CURDIR/bin && rm -rf $CURDIR/bin
    mkdir $CURDIR/bin
    go mod tidy
    go build -o $CURDIR/bin/${filename}
echo "${RED_BOLD}Build c-compiler${NORMAL}"
    test -e serializer && rm -f serializer
    test -e llvm_wrapper && rm -f llvm_wrapper
    sh $CURDIR/../serialization/compile.sh $1
    cp $CURDIR/../serialization/serializer $CURDIR/bin/serializer
    cd $CURDIR/../../../
    test -d $CURDIR/../../../build && rm -rf $CURDIR/../../../build
    mkdir $CURDIR/../../../build
    cd $CURDIR/../../../build
    cmake -DIRONLY=ON .. 1>/dev/null && make 1>/dev/null
    test -e "zjucc_backend" && echo "Backend build success"
    cp zjucc_backend $CURDIR/bin/llvm_wrapper

echo "${RED_BOLD}Backend start${NORMAL}"
    cd $CURDIR/bin
    export GIN_MODE=release
    exec $CURDIR/bin/${filename}