#!/usr/bin/env bash

set -e

BASEDIR=$(pwd)
BUILDDIR=/tmp/build-$1

$BASEDIR/.ci/common/strip-ultravnc-sources.sh

rm -rf $BUILDDIR
mkdir $BUILDDIR
cd $BUILDDIR

if [ "${MSYSTEM:-}" = "UCRT64" ] && [ -x /ucrt64/bin/qt-cmake ]; then
	PREFIX=/ucrt64
elif [ "${MSYSTEM:-}" = "MINGW64" ] && [ -x /mingw64/bin/qt-cmake ]; then
	PREFIX=/mingw64
elif [ -n "${MINGW_PREFIX:-}" ] && [ -x "${MINGW_PREFIX}/bin/qt-cmake" ]; then
	PREFIX="${MINGW_PREFIX}"
else
	PREFIX="/usr/$1-w64-mingw32"
fi
$PREFIX/bin/qt-cmake $BASEDIR \
	-G Ninja \
	-DQT_TRANSLATIONS_DIR=$PREFIX/translations \
	$CMAKE_FLAGS

if [ -z "$2" ] ; then
	ninja windows-binaries
else
	ninja ${@:2}
fi

mv vyrja-classroom-*setup.exe veyon-*win* $BASEDIR 2>/dev/null || true
