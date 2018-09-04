Primecoin integration/staging tree
==================================

http://primecoin.org

Copyright (c) 2013 Primecoin Developers

Copyright (c) 2009-2013 Bitcoin Developers

Copyright (c) 2011-2013 PPCoin Developers

What is Primecoin?
------------------

Primecoin is an experimental cryptocurrency that introduces the first
scientific computing proof-of-work to cryptocurrency technology. Primecoin's
proof-of-work is an innovative design based on searching for prime number
chains, providing potential scientific value in addition to minting and
security for the network. Similar to Bitcoin, Primecoin enables instant payments
to anyone, anywhere in the world. It also uses peer-to-peer technology to
operate with no central authority: managing transactions and issuing money are
carried out collectively by the network. Primecoin is also the name of the open
source software which enables the use of this currency.

For more information, as well as an immediately useable, binary version of
the Primecoin client sofware, see http://primecoin.org.

License
-------

Primecoin is released under conditional MIT license. See  COPYING for more
information.

Development process
-------------------

Developers work in their own trees, then submit pull requests when they think
their feature or bug fix is ready.

If it is a simple/trivial/non-controversial change, then one of the Bitcoin
development team members simply pulls it.

If it is a *more complicated or potentially controversial* change, then the patch
submitter will be asked to start a discussion (if they haven't already) on the
ppcoin/primecoin forum (https://talk.peercoin.net).

The patch will be accepted if there is broad consensus that it is a good thing.
Developers should expect to rework and resubmit patches if the code doesn't
match the project's coding conventions (see `doc/coding.md`) or are
controversial.

The `master` branch is regularly built and tested, but is not guaranteed to be
completely stable. [Tags](https://github.com/primecoin/primecoin/tags) are
created regularly to indicate new official, stable release versions of
Primecoin.

### build step
#### ubuntu 16.04

```
sudo apt-get update
sudo apt-get install build-essential
sudo apt-get install -y build-essential autoconf libtool pkg-config libboost-all-dev libssl-dev libevent-dev libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools libprotobuf-dev protobuf-compiler libqrencode-dev libminiupnpc-dev curl

cd depends
make
cd ..
./autogen.sh
./configure --prefix=`pwd`/depends/x86_64-pc-linux-gnu --disable-shared LDFLAGS="-static-libstdc++"
make STATIC=all
```

#### cross compile for win64/32
if want to compile win32 version just need change x86_64-w64-mingw32
```
sudo apt-get update
sudo apt-get install build-essential
sudo apt-get install -y autoconf libtool pkg-config libboost-all-dev libssl-dev libevent-dev libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools libprotobuf-dev protobuf-compiler libqrencode-dev libminiupnpc-dev curl

// cross compile specific depends
sudo apt install -y autotools-dev automake pkg-config bsdmainutils curl git g++-mingw-w64-x86-64 nsis

sudo apt install software-properties-common
sudo add-apt-repository "deb http://archive.ubuntu.com/ubuntu zesty universe"
sudo apt update
sudo apt upgrade
sudo update-alternatives --config x86_64-w64-mingw32-g++ # Set the default mingw32 g++ compiler option to posix.

PATH=$(echo "$PATH" | sed -e 's/:\/mnt.*//g') # strip out problematic Windows %PATH% imported var

cd depends
make HOST=x86_64-w64-mingw32
cd ..
./autogen.sh
./configure --prefix=`pwd`/depends/x86_64-w64-mingw32  --disable-shared
make STATIC=all

// package installer
makensis share/setup.nsi
```

cross compile mac should be similar step. not test yet.

Testing
-------

Testing and code review is the bottleneck for development; we get more pull
requests than we can review and test. Please be patient and help out, and
remember this is a security-critical project where any mistake might cost people
lots of money.

### Automated Testing

Developers are strongly encouraged to write unit tests for new code, and to
submit new unit tests for old code.

Unit tests for the core code are in `src/test/`. To compile and run them:

    cd src; make -f makefile.unix test

Unit tests for the GUI code are in `src/qt/test/`. To compile and run them:

    qmake BITCOIN_QT_TEST=1 -o Makefile.test bitcoin-qt.pro
    make -f Makefile.test
    ./bitcoin-qt_test

### Manual Quality Assurance (QA) Testing

Large changes should have a test plan, and should be tested by somebody other
than the developer who wrote the code.

See https://github.com/bitcoin/QA/ for how to create a test plan.
