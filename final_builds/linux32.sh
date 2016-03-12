#!/bin/bash

set -e

mkdir -p files
rm -rf ../build
mkdir ../build && pushd ../build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/home/max/Qt/5.4/gcc/lib/cmake ..
make -j2
make install
popd

pushd ../linux_pkg
./build_linux_pkgs32.sh
mv *rpm ../final_builds/files
mv *deb ../final_builds/files
mv *gz ../final_builds/files
popd