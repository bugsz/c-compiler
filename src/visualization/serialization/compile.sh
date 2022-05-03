cd ../../frontend
sh clean.sh
./configure
make
cd ../visualization/serialization
g++ *.cpp -Iinclude -I../../frontend -Llib -L../../frontend -lzjucc -lfmt -std=c++17 -o serializer