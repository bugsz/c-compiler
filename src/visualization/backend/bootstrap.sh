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
    sh $CURDIR/build.sh $1

echo "${RED_BOLD}Backend start${NORMAL}"
    cd $CURDIR/bin
    export GIN_MODE=release
    touch $CURDIR/bin/update.sh 
    echo "git pull &&" >> $CURDIR/bin/update.sh
    echo "sh ../build.sh NEW_BUILD &&" >> $CURDIR/bin/update.sh
    echo "cd ../../frontend &&" >> $CURDIR/bin/update.sh
    echo "yarn install &&" >> $CURDIR/bin/update.sh 
    echo "yarn build" >> $CURDIR/bin/update.sh
    echo "cp -r ../../../compatible-header/*.h ./" >> $CURDIR/bin/update.sh
    cp -r $CURDIR/../../compatible-header/* $CURDIR/bin
    exec nohup $CURDIR/bin/${filename} &