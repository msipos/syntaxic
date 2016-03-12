#!/bin/bash

# Run from build directory (where Syntaxic.app is)

set -e

VERSION=0.9.3

rm -f Syntaxic.dmg
hdiutil create -megabytes 50 -fs HFS+ -volname syntaxic_app Syntaxic.dmg
hdiutil mount Syntaxic.dmg
cp -R Syntaxic.app /Volumes/syntaxic_app/
ln -s /Applications /Volumes/syntaxic_app/Applications
hdiutil detach /Volumes/syntaxic_app
hdiutil convert -format UDZO -o Syntaxic2.dmg Syntaxic.dmg
mv Syntaxic2.dmg Syntaxic-${VERSION}.dmg
rm Syntaxic.dmg