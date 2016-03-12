#!/bin/bash

set -e

rm -rf re2
git clone /home/max/repos/re2 re2

pushd re2

mkdir -p build
pushd build

cmake ..
make

popd

mkdir -p ../../include/re2
cp -v re2/*.h ../../include/re2
cp -v build/libre2.a ../../lib

popd
