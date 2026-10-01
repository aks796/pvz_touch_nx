#!/bin/sh
# Build pvz_touch_nx.nro (the launcher) with the runtime's launcher build
# (devkitPro's 64-bit toolchain container). Build the wrapper first
# (../build.sh): the NRO carries ../pvz_nx.nsp and ../pvz_nx.build, and the
# English files (romfs_extras.sh).
HERE="$(cd "$(dirname "$0")" && pwd)"
LAUNCHER_DIR="$HERE" PAYLOAD=pvz_nx exec "$HERE/../runtime/launcher/build.sh" "$@"
