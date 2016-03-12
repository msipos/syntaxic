#!/bin/bash

set -e

rm -rf lmgen-src

mkdir -v lmgen-src
mkdir -vp lmgen-src/src/core
cp -v ../src/lmgenmain.* lmgen-src/src/
cp -v ../src/lmgen.* lmgen-src/src/
cp -v ../src/lm.* lmgen-src/src/
cp -v ../src/core/utf8_util* lmgen-src/src/core/
cp -v ../third-party/include/utf8.h lmgen-src/src/
cp -rv ../third-party/include/utf8 lmgen-src/src/
cp -v build_lmgen.sh lmgen-src/

pushd lmgen-src
./build_lmgen.sh
popd

tar czvf lmgen-src.tar.gz lmgen-src/