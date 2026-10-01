# romfs_extras.sh -- sourced by the runtime's launcher/build.sh (HERE, PORT,
# ROMFS set). The English files: PVZ_ENGLISH_PACK names the pack made by
# ../tools/make_english_pack.py (default: ../../english_pack/PvZ Touch
# English.apk, outside the project, so it is never in the repository). The NRO
# carries it as romfs:/english.apk; the game copies it into its folder on the
# first start (source/pvz_main.c). Without one the NRO is built without it
# (players then add an English APK themselves).
PACK="${PVZ_ENGLISH_PACK:-$PORT/../english_pack/PvZ Touch English.apk}"
if [ -f "$PACK" ]; then
  if ! cmp -s "$PACK" "$ROMFS/english.apk"; then
    cp "$PACK" "$ROMFS/english.apk"
    rm -f "$HERE/pvz_touch_nx.nro" # repack the NRO with it
  fi
  echo "English files: $PACK"
elif [ -f "$ROMFS/english.apk" ]; then
  rm -f "$ROMFS/english.apk" "$HERE/pvz_touch_nx.nro"
  echo "English files: none (no $PACK)"
fi
