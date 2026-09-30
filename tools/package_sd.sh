#!/bin/sh
# package_sd.sh -- assemble SD_CARD/ (and SD_CARD.zip) for a test on a Switch.
#
#   tools/package_sd.sh [path/to/your/game.apk [path/to/your/english.apk]]
#
# Builds the wrapper and the launcher, then lays out what goes on the card:
#   SD_CARD/switch/pvz_touch_nx/pvz_touch_nx.nro
#   SD_CARD/switch/pvz_touch_nx/<the game APK>     (only if an APK path is given: your
#                                                  own copy, under its own name)
#   SD_CARD/switch/pvz_touch_nx/<the English APK>  (likewise: the English 4.0.5 APK)
#   SD_CARD/README_FIRST.txt
# The wrapper tells the two APKs apart by their contents (source/pvz_apks.c).
# The ExeFS override is not included: the launcher writes it for whichever
# sphaira forwarder it is started from.
set -e
HERE="$(cd "$(dirname "$0")/.." && pwd)"
cd "$HERE"
./build.sh
launcher/build.sh
if [ -n "$1" ]; then
  python3 tools/test_setup.py "$1" launcher/pvz_touch_nx.nro
fi
if [ -n "$2" ]; then
  out=$(python3 tools/test_english.py "$1" "$2") || { echo "$out"; exit 1; }
  echo "$out" | tail -n 1
  out=$(python3 tools/test_apks.py "$1" "$2" 2>&1) || { echo "$out"; exit 1; }
  echo "APKs and the old folder: $(echo "$out" | tail -n 1)"
fi

rm -rf SD_CARD SD_CARD.zip
mkdir -p SD_CARD/switch/pvz_touch_nx
cp launcher/pvz_touch_nx.nro SD_CARD/switch/pvz_touch_nx/
cp tools/README_FIRST.txt SD_CARD/
if [ -n "$1" ]; then
  cp -p "$1" SD_CARD/switch/pvz_touch_nx/  # -p: keep its date (it is the same file every build)
fi
if [ -n "$2" ]; then
  cp -p "$2" SD_CARD/switch/pvz_touch_nx/
fi
(cd SD_CARD && zip -qr ../SD_CARD.zip .)
echo "build $(cat pvz_nx.build): SD_CARD/ and SD_CARD.zip ready"
