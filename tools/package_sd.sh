#!/bin/sh
# package_sd.sh -- assemble SD_CARD/ (and SD_CARD.zip) for a test on a Switch.
#
#   tools/package_sd.sh [path/to/your/game.apk [path/to/your/english.apk]]
#
# Builds the wrapper and the launcher, then lays out what goes on the card:
#   SD_CARD/switch/pvz_touch_nx/pvz_touch_nx.nro
#   SD_CARD/switch/pvz_touch_nx/<the game APK>   (only if an APK path is given: your
#                                                own copy, under its own name)
#   SD_CARD/README_FIRST.txt
# With the English APK: the English pack (tools/make_english_pack.py) is made
# from it into ../english_pack/ (outside the project) and built into the NRO,
# which the game copies out on its first start; the English APK itself is not
# put on the card.
# The ExeFS override is not included: the launcher writes it for whichever
# sphaira forwarder it is started from.
set -e
HERE="$(cd "$(dirname "$0")/.." && pwd)"
cd "$HERE"
./build.sh
if [ -n "$2" ]; then
  mkdir -p ../english_pack
  python3 tools/make_english_pack.py "$1" "$2" -o "../english_pack/PvZ Touch English.apk"
fi
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
(cd SD_CARD && zip -qr ../SD_CARD.zip .)
echo "build $(cat pvz_nx.build): SD_CARD/ and SD_CARD.zip ready"
