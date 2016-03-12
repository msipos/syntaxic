#!/bin/bash

set -e

PKGVERSION=0.9.3

rm -rf etc/
rm -rf opt/
rm -rf usr/
rm -rf syntaxic-0*
rm -f *deb *rpm *tar.gz

mkdir -p opt/syntaxic/platforms
mkdir -p opt/syntaxic/iconengines
mkdir -p opt/syntaxic/platformthemes
mkdir -p etc/profile.d
mkdir -p usr/share/applications

cp -v syntaxic.sh etc/profile.d
cp -v ../build/prefix/syntaxic opt/syntaxic/
cp -v ../SYNTAXIC_LICENSE opt/syntaxic/
cp -v ../SYNTAXIC_HELP opt/syntaxic
cp -v ../SYNTAXIC_WELCOME opt/syntaxic
cp -v ../resources/icon.png opt/syntaxic/
cp -v syntaxic.desktop usr/share/applications
cp -rv ../meta/* opt/syntaxic/
cp -v ../bin/syntaxic_local_wrapper.linux64 opt/syntaxic/syntaxic_local_wrapper
cp -v ~/Qt/5.4/gcc_64/plugins/platforms/libqxcb.so opt/syntaxic/platforms/
cp -v ~/Qt/5.4/gcc_64/plugins/iconengines/* opt/syntaxic/iconengines/
cp -v ~/Qt/5.4/gcc_64/plugins/platformthemes/* opt/syntaxic/platformthemes/
cp -v ~/Qt/5.4/gcc_64/lib/libQt5Core.so opt/syntaxic/
cp -v ~/Qt/5.4/gcc_64/lib/libQt5Svg.so opt/syntaxic/
cp -v ~/Qt/5.4/gcc_64/lib/libQt5Network.so opt/syntaxic/
cp -v ~/Qt/5.4/gcc_64/lib/libQt5DBus.so opt/syntaxic/
cp -v ~/Qt/5.4/gcc_64/lib/libQt5Gui.so opt/syntaxic/
cp -v ~/Qt/5.4/gcc_64/lib/libQt5Widgets.so opt/syntaxic/
cp -v ~/Qt/5.4/gcc_64/lib/libicui18n.so.53.1 opt/syntaxic/libicui18n.so.53
cp -v ~/Qt/5.4/gcc_64/lib/libicuuc.so.53.1 opt/syntaxic/libicuuc.so.53
cp -v ~/Qt/5.4/gcc_64/lib/libicudata.so.53.1 opt/syntaxic/libicudata.so.53

pushd opt/syntaxic
chmod -x lib*
ln -s libQt5Widgets.so libQt5Widgets.so.5
ln -s libQt5Gui.so libQt5Gui.so.5
ln -s libQt5Core.so libQt5Core.so.5
ln -s libQt5DBus.so libQt5DBus.so.5
ln -s libQt5Svg.so libQt5Svg.so.5
ln -s libQt5Network.so libQt5Network.so.5
strip ./syntaxic
strip ./libicui18n.so.53
strip ./libicuuc.so.53
strip ./libicudata.so.53
popd

chmod -R g-w etc/
chmod -R g-w opt/
chmod -R g-w usr/

DESCRIPTION="Keyboard-friendly text editor with syntax support."
FLAGS="-s dir -n syntaxic -v $PKGVERSION --vendor k-partite --url http://syntaxiceditor.com --license Commercial opt/ usr/ etc/"
echo "Making deb..."
fpm -t deb --description "$DESCRIPTION" -m "Maksim Sipos <support@kpartite.com>" $FLAGS
echo "Making rpm..."
fpm -t rpm --description "$DESCRIPTION" -m "Maksim Sipos <support@kpartite.com>" $FLAGS
echo "Making tgz..."
mkdir syntaxic-$PKGVERSION-64bit
cp -R opt/syntaxic/* syntaxic-$PKGVERSION-64bit/
cp syntaxic.desktop syntaxic-$PKGVERSION-64bit/
tar czvf syntaxic-$PKGVERSION-64bit.tar.gz syntaxic-$PKGVERSION-64bit/