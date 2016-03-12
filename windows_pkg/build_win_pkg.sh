#!/bin/bash

set -e

QTDIR=C:/Qt/5.4/msvc2013/bin
VERSION=0.9.3
OUTDIR=Syntaxic-${VERSION}

echo "Building Windows package..."

rm -vrf ${OUTDIR}
mkdir -v ${OUTDIR}

# Copy all the ingredients
cp -v ../build/Release/utests.exe ${OUTDIR}
cp -v ../bin/plink.exe ${OUTDIR}
cp -v ${QTDIR}/Qt5Core.dll ${OUTDIR}
cp -v ${QTDIR}/icuin53.dll ${OUTDIR}
cp -v ${QTDIR}/icuuc53.dll ${OUTDIR}
cp -v ${QTDIR}/icudt53.dll ${OUTDIR}
cp -v ${QTDIR}/Qt5Widgets.dll ${OUTDIR}
cp -v ${QTDIR}/Qt5Gui.dll ${OUTDIR}
cp -v ${QTDIR}/Qt5Network.dll ${OUTDIR}
mkdir -p ${OUTDIR}/platforms
cp -v ${QTDIR}/../plugins/platforms/qwindows.dll ${OUTDIR}/platforms

# Run a test
pushd ${OUTDIR}
cp -vr ../../test_files .
./utests.exe
rm -vr ./test_files/
rm -v ./utests.exe
popd

# Copy remaining
cp -v ../build/Release/syntaxic.exe ${OUTDIR}
cp -v C:/Windows/System32/msvcr120.dll ${OUTDIR}
cp -v C:/Windows/System32/msvcp120.dll ${OUTDIR}
cp -v ../SYNTAXIC_LICENSE ${OUTDIR}
cp -v ../SYNTAXIC_HELP ${OUTDIR}
cp -v ../SYNTAXIC_WELCOME ${OUTDIR}
cp -rv ../meta/* ${OUTDIR}

# Test running the executable
${OUTDIR}/syntaxic.exe