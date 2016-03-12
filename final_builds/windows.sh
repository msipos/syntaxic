#!/bin/bash

set -e

mkdir -p files
rm -rf ../build
mkdir ../build && pushd ../build

cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/5.4/msvc2013/lib/cmake ..
C:/Program\ Files\ \(x86\)/MSBuild/12.0/bin/MSBuild.exe utests.vcxproj //p:Configuration=Release
C:/Program\ Files\ \(x86\)/MSBuild/12.0/bin/MSBuild.exe syntaxic.vcxproj //p:Configuration=Release
popd

pushd ../windows_pkg
./build_win_pkg.sh

#TODO: ZIP

popd