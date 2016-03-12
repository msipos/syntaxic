#!/bin/bash

echo "Remember to cp new meta dir."
sleep 3

set -e

rm -rf ../build
mkdir ../build
mkdir -p files
pushd ../build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/Users/max/Qt5.4.1/5.4/clang_64/lib/cmake ..
make -j2
make install
../mac_pkg/make_mac_dmg.sh
popd

mv ../build/*.dmg files/
