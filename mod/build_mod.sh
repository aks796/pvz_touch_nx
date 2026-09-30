#!/bin/sh
# build_mod.sh -- build libHomura.so (the PvZ TV Touch mod) from mod/src with
# the Android NDK r27d, the toolchain the mod's own Android build uses
# (app/build.gradle: ndkVersion 27.3.13750724, minSdk 21, armeabi-v7a,
# -DPVZ_VERSION=115 for the 1.1.5 game, Release).
#
#   mod/build_mod.sh            -> mod/out/libHomura.so
#
# ANDROID_NDK: an NDK directory. Otherwise the macOS image
# ../toolchain/android-ndk-r27d-darwin.dmg (next to pvztouch_nx) is mounted
# read-only for the build and detached afterwards.
#
# mod/ is a git repository: the tags upstream-7f01553 (the 1.1.5-260924 mod)
# and upstream-e91bc9a (the 260925 nightly, online protocol 3199) are the mod
# as published (github.com/ZombieYetis/PlantsVsZombies-AndroidTV, GPL-3.0),
# each building byte for byte the APK's code and data; the commits after
# upstream-e91bc9a are this port's changes (`git -C mod diff upstream-e91bc9a`).
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
DMG="${NDK_DMG:-$HERE/../../toolchain/android-ndk-r27d-darwin.dmg}"
if [ -z "$ANDROID_NDK" ]; then
  # a fixed mount point: CMake's cache keeps the compiler's path
  MNT=/tmp/pvz-ndk-r27d
  if [ ! -d "$MNT/AndroidNDK13750724.app" ]; then
    mkdir -p "$MNT"
    hdiutil attach -nobrowse -readonly -mountpoint "$MNT" "$DMG" >/dev/null
    trap 'hdiutil detach "$MNT" >/dev/null 2>&1' EXIT
  fi
  ANDROID_NDK="$MNT/AndroidNDK13750724.app/Contents/NDK"
fi
BUILD="$HERE/build"
cmake -S "$HERE/src" -B "$BUILD" -G "Unix Makefiles" \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=armeabi-v7a -DANDROID_PLATFORM=android-21 \
  -DCMAKE_BUILD_TYPE=Release -DPVZ_VERSION=115 >/dev/null
cmake --build "$BUILD" -j"$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
mkdir -p "$HERE/out"
BIN="$(ls -d "$ANDROID_NDK"/toolchains/llvm/prebuilt/*/bin | head -1)"
cp "$BUILD/libHomura.so" "$HERE/out/libHomura.debug.so"   # symbols, for crash logs
"$BIN/llvm-strip" --strip-unneeded -o "$HERE/out/libHomura.so" "$BUILD/libHomura.so"
ls -l "$HERE/out/libHomura.so"
