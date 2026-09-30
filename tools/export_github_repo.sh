#!/bin/sh
# export_github_repo.sh -- refresh github_repo/ (what goes on GitHub) from the
# project: source, build files, tools, mod source, docs.
#
#   tools/export_github_repo.sh
#
# Never copied: build outputs, SD_CARD, debug logs, portlibs32 (the 32-bit
# Mesa and FFmpeg), the mod's build folders and history, APKs (game files).
# Never touched in github_repo/: its .git (the published repository) and its
# .gitignore. Files deleted from the project are deleted there too.
set -e
HERE="$(cd "$(dirname "$0")/.." && pwd)"
cd "$HERE"
mkdir -p github_repo
rsync -a --delete \
  --exclude='/github_repo' --exclude='/SD_CARD' --exclude='/SD_CARD.zip' --exclude='/build/' --exclude='/debug/' \
  --exclude='/portlibs32/' --exclude='/pvz_nx.elf' --exclude='/pvz_nx.nsp' --exclude='/pvz_nx.build' \
  --exclude='/PLAN.md' --exclude='/newicon*.jpg' --exclude='.DS_Store' --exclude='__pycache__' --exclude='*.pyc' \
  --exclude='/launcher/build/' --exclude='/launcher/romfs/' --exclude='/launcher/*.nro' --exclude='/launcher/*.elf' \
  --exclude='/launcher/*.nacp' --exclude='/mod/build/' --exclude='/mod/out/' --exclude='/mod/.git' \
  --exclude='*.orig' --exclude='*.bak' --exclude='*~' --exclude='*.apk' \
  --exclude='/.git' --exclude='/.gitignore' \
  ./ github_repo/
echo "github_repo/: $(find github_repo -path github_repo/.git -prune -o -type f -print | wc -l | tr -d ' ') files"
