#!/usr/bin/env bash

set -e

BASEDIR=$(pwd)
BUILDDIR=/tmp/build-$1

$BASEDIR/.ci/common/strip-ultravnc-sources.sh

rm -rf $BUILDDIR
mkdir $BUILDDIR
cd $BUILDDIR

if [ "${MSYSTEM:-}" = "UCRT64" ] && [ -d /ucrt64 ]; then
	PREFIX=/ucrt64
elif [ "${MSYSTEM:-}" = "MINGW64" ] && [ -d /mingw64 ]; then
	PREFIX=/mingw64
elif [ -n "${MINGW_PREFIX:-}" ] && [ -x "${MINGW_PREFIX}/bin/qt-cmake" ]; then
	PREFIX="${MINGW_PREFIX}"
else
	PREFIX="/usr/$1-w64-mingw32"
fi

export MINGW_PREFIX="$PREFIX"
export MINGW_TARGET="${MINGW_TARGET:-$1-w64-mingw32}"
export MINGW_TOOL_PREFIX="${MINGW_TOOL_PREFIX:-$1-w64-mingw32-}"

if [ -x "$PREFIX/bin/qt-cmake" ]; then
	CMAKE_CMD=("$PREFIX/bin/qt-cmake")
else
	CMAKE_CMD=(cmake "-DCMAKE_PREFIX_PATH=$PREFIX")
fi

"${CMAKE_CMD[@]}" "$BASEDIR" \
	-G Ninja \
	-DMINGW_PREFIX="$PREFIX" \
	-DMINGW_TARGET="$MINGW_TARGET" \
	-DMINGW_TOOL_PREFIX="$MINGW_TOOL_PREFIX" \
	-DQT_TRANSLATIONS_DIR=$PREFIX/translations \
	$CMAKE_FLAGS

if [ -z "$2" ] ; then
	ninja windows-binaries
else
	ninja ${@:2}
fi

mv vyrja-classroom-*setup.exe veyon-*win* $BASEDIR 2>/dev/null || true
