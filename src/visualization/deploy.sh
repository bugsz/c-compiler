CURDIR=$(cd $(dirname $0); pwd)

cd $CURDIR/frontend
yarn install
yarn build

cd $CURDIR/backend
sh bootstrap.sh NEW_BUILD

