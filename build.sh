#!/bin/sh
export CFLAGS="-march=native -ftree-vectorize"
export CXXFLAGS="-march=native -ftree-vectorize"
mkdir build
cd build
../configure --prefix=/home/user/opt/brlcad/ --enable-all
make install -j2
