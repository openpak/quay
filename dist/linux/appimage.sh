#!/bin/bash

if [ "$1" == 'X64' ]; then CPU_ARCH="x86_64"; else CPU_ARCH="aarch64"; fi

if [[ -z "${GITHUB_WORKSPACE}" ]]; then
	export GITHUB_WORKSPACE="."
fi

curl -sSfLO "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-"${CPU_ARCH}".AppImage"
chmod a+x linuxdeploy*.AppImage
curl -sSfL https://github.com"$(curl https://github.com/probonopd/go-appimage/releases/expanded_assets/continuous | grep "mkappimage-.*-"${CPU_ARCH}".AppImage" | head -n 1 | cut -d '"' -f 2)" -o mkappimage.AppImage
chmod a+x mkappimage.AppImage
curl -sSfLO "https://raw.githubusercontent.com/linuxdeploy/linuxdeploy-plugin-gtk/master/linuxdeploy-plugin-gtk.sh"
chmod a+x linuxdeploy-plugin-gtk.sh
curl -sSfLO "https://github.com/darealshinji/linuxdeploy-plugin-checkrt/releases/download/continuous/linuxdeploy-plugin-checkrt.sh"
chmod a+x linuxdeploy-plugin-checkrt.sh

if [[ ! -e /usr/lib/"${CPU_ARCH}"-linux-gnu ]]; then
	sed -i 's#lib\/"${CPU_ARCH}"-linux-gnu#lib64#g' linuxdeploy-plugin-gtk.sh
fi

mkdir -p AppDir/usr/bin
mkdir -p AppDir/usr/share/Quay
mkdir -p AppDir/usr/share/applications
mkdir -p AppDir/usr/share/icons/hicolor/128x128/apps
mkdir -p AppDir/usr/share/metainfo
mkdir -p AppDir/usr/lib

cp dist/linux/org.openpak.quay.{desktop,png} AppDir/
cp dist/linux/org.openpak.quay.metainfo.xml AppDir/usr/share/metainfo/org.openpak.quay.appdata.xml

cp -r bin/* AppDir/usr/share/Quay

mv AppDir/usr/share/Quay/Quay AppDir/usr/bin/
chmod +x AppDir/usr/bin/Quay

cp /usr/lib/"${CPU_ARCH}"-linux-gnu/{libsepol.so.1,libffi.so.7,libpcre.so.3,libGLU.so.1,libthai.so.0} AppDir/usr/lib

export UPD_INFO="gh-releases-zsync|openpak|quay|nightly|Quay.AppImage.zsync"
export NO_STRIP=1
./linuxdeploy-"${CPU_ARCH}".AppImage --appimage-extract-and-run \
  --appdir="${GITHUB_WORKSPACE}"/AppDir/ \
  -d "${GITHUB_WORKSPACE}"/AppDir/org.openpak.quay.desktop \
  -i "${GITHUB_WORKSPACE}"/AppDir/org.openpak.quay.png \
  -e "${GITHUB_WORKSPACE}"/AppDir/usr/bin/Quay \
  --plugin gtk \
  --plugin checkrt

if ! GITVERSION="$(git rev-parse --short HEAD 2>/dev/null)"; then
	GITVERSION=experimental
fi
echo "Quay Version Quay-${GITVERSION}"

rm AppDir/usr/lib/libwayland-client.so.0
echo -e "export LC_ALL=C\nexport FONTCONFIG_PATH=/etc/fonts" >> AppDir/apprun-hooks/linuxdeploy-plugin-gtk.sh
VERSION="${GITVERSION}" ./mkappimage.AppImage --appimage-extract-and-run "${GITHUB_WORKSPACE}"/AppDir

mkdir -p "${GITHUB_WORKSPACE}"/artifacts/
mv Quay-"${GITVERSION}"-"${CPU_ARCH}".AppImage "${GITHUB_WORKSPACE}"/artifacts/
