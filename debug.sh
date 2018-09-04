make clean
./autogen.sh
./configure --enable-debug
make
mkdir bin
cp ./src/primecoind ./src/primecoin-* ./src/qt/primecoin-qt ./bin
