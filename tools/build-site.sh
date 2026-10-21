#!/usr/bin/env bash
# Assembles site/ for dwcode.pages.dev: the browser editor demo, screenshots,
# theme data and the brand download. Run from anywhere; needs npm, ImageMagick
# and python3. Pass --shots to recapture the app screenshots first (builds the
# offscreen screenshot tool in build/shots).
set -euo pipefail
cd "$(dirname "$0")/.."

if [[ "${1:-}" == "--shots" ]]; then
  ./bin/build
  mkdir -p build/shots
  (cd build/shots && qmake6 ../../tests/shots.pro && make -j"$(nproc)" && ./site-shots)
fi

# Browser demo: the same Monaco bundle the desktop app loads, without the
# native WebChannel, so it edits sample files in memory only.
npm run build
rm -rf site/demo
cp -r web/dist site/demo

# Screenshots: 2x PNG captures -> WebP, full size plus a 1440 px variant.
raw=build/site-shots
[[ -d "$raw" ]] || { echo "No $raw; run with --shots" >&2; exit 1; }
for png in "$raw"/*.png; do
  name=$(basename "$png" .png)
  magick "$png" -quality 82 "site/screenshots/$name@2x.webp"
  magick "$png" -resize 1440x -quality 82 "site/screenshots/$name.webp"
done

python3 tools/make-site-background.py

mkdir -p site/data
cp src/data/themes.json site/data/themes.json

# anime.js, served locally (pinned in package.json).
mkdir -p site/vendor
cp node_modules/animejs/dist/bundles/anime.esm.min.js site/vendor/anime.esm.min.js

du -sh site
