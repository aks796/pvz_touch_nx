#!/bin/sh
# Build pvz_touch_nx.nro (the launcher) in devkitPro's 64-bit toolchain
# container. Build the wrapper first (../build.sh): the NRO carries
# ../pvz_nx.nsp and ../pvz_nx.build.
#
# The English files: PVZ_ENGLISH_PACK names the pack made by
# ../tools/make_english_pack.py (default: ../../english_pack/PvZ Touch
# English.apk, outside the project, so it is never in the repository). The NRO
# carries it as romfs:/english.apk; the game copies it into its folder on the
# first start. Without one the NRO is built without it (players then add an
# English APK themselves).
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
[ -f "$HERE/../pvz_nx.nsp" ] && [ -f "$HERE/../pvz_nx.build" ] || { echo "build the wrapper first (../build.sh)"; exit 1; }
PACK="${PVZ_ENGLISH_PACK:-$HERE/../../english_pack/PvZ Touch English.apk}"
mkdir -p "$HERE/romfs"
if [ -f "$PACK" ]; then
  if ! cmp -s "$PACK" "$HERE/romfs/english.apk"; then
    cp "$PACK" "$HERE/romfs/english.apk"
    rm -f "$HERE/pvz_touch_nx.nro" # repack the NRO with it
  fi
  echo "English files: $PACK"
elif [ -f "$HERE/romfs/english.apk" ]; then
  rm -f "$HERE/romfs/english.apk" "$HERE/pvz_touch_nx.nro"
  echo "English files: none (no $PACK)"
fi
exec docker run --rm --platform linux/amd64 \
  -v "$HERE/..:/work" -w /work/launcher devkitpro/devkita64:latest \
  bash -lc "make -j\$(nproc) $*"
