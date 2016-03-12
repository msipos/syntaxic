#!/bin/bash

set -e

mkdir -p files
rm -rf ../build
mkdir ../build && pushd ../build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/home/max/Qt/5.4/gcc_64/lib/cmake ..
make -j4
make install
popd

pushd ../linux_pkg
./build_linux_pkgs64.sh
mv *rpm ../final_builds/files
mv *deb ../final_builds/files
mv *gz ../final_builds/files
popd