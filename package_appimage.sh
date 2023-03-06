#!/usr/bin/env bash
# Example: ./package_appimage.sh build/bin/clang-tidy build/lib/clang/13.0.0/
# Please build binaries on centos7 for the best compatibility

set -uexo pipefail

if ! [ -x "$(command -v convert)" ]; then
  echo 'Error: convert is not installed. Please install imagemagick (run `apt-get install imagemagick` if you are on Ubuntu)' >&2
  exit 1
fi

if [ ! -f linuxdeploy-x86_64.AppImage ]
then
  wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
fi
if [ ! -f linuxdeploy-plugin-appimage-x86_64.AppImage ]
then
  wget https://github.com/linuxdeploy/linuxdeploy-plugin-appimage/releases/download/continuous/linuxdeploy-plugin-appimage-x86_64.AppImage
fi

chmod +x linuxdeploy-*.AppImage
rm -rf appimage/appdir
mkdir -p appimage/appdir/usr/bin
cp $1 appimage/appdir/usr/bin/
mkdir -p appimage/appdir/usr/lib/clang/
cp -r $2 appimage/appdir/usr/lib/clang/

bn=`basename $1`

echo "[Desktop Entry]
Name=$bn
Exec=$bn
Icon=$bn
Type=Application
Terminal=true
Categories=Development;" > appimage/$bn.desktop

convert -size 32x32 xc:white appimage/$bn.png

./linuxdeploy-x86_64.AppImage --appdir appimage/appdir -d appimage/$bn.desktop -i appimage/$bn.png --output appimage

# rm -rf appimage/appdir
