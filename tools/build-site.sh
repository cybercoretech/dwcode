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
# Conventional root favicon for direct links and browsers requesting /favicon.ico.
cp site/assets/favicon.ico site/favicon.ico
cp site/assets/favicon.svg site/favicon.svg

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
# Built-in themes plus pack contributions, so the gallery matches the app.
node tools/theme-catalog.mjs > site/data/themes.json

# Extension catalog: paint the committed catalog data into repository.html and
# cross-check data ↔ HTML ↔ FOSS_EXTENSION_REPOSITORY.md (offline). Refreshing
# the data itself is `npm run catalog:build` + `npm run catalog:render`, both of
# which need the registry, so a deploy never silently re-sources the catalog.
node tools/catalog.mjs render
node tools/catalog.mjs check

# anime.js, served locally (pinned in package.json).
mkdir -p site/vendor
cp node_modules/animejs/dist/bundles/anime.esm.min.js site/vendor/anime.esm.min.js

du -sh site
