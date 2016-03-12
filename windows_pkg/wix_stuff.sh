#!/bin/bash

set -e

SYNTAXIC_VER=0.9.3

rm -f syntaxic.msi
rm -rf WixSourceDir
cp -R Syntaxic-${SYNTAXIC_VER} WixSourceDir

export PATH=$PATH:"/c/Program Files (x86)/WiX Toolset v3.9/bin"
#heat dir WixSourceDir -ag -scom -suid -sfrag -srd -out fragments.wxs
candle syntaxic.wxs
light -ext WixUIExtension -cultures:en-us syntaxic.wixobj

rm -f *.wixobj
rm -f *.wixpdb
mv syntaxic.msi Syntaxic-${SYNTAXIC_VER}.msi