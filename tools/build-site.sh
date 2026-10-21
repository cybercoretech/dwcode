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

# Web-sized logo renders for the page.
magick site/assets/railshift.png -resize 720x -quality 88 site/assets/railshift-hero.webp
magick site/assets/railshift.png -trim -resize 80x -quality 90 site/assets/railshift-mark.webp

mkdir -p site/data
cp src/data/themes.json site/data/themes.json

[[ -f brand/dwcode-railshift-assets.zip ]] || python3 tools/build-brand-assets.py
mkdir -p site/downloads
cp brand/dwcode-railshift-assets.zip site/downloads/

du -sh site
