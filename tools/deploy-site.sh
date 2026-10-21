#!/usr/bin/env bash
# Builds site/ and uploads it to the Cloudflare Pages project "dwcode"
# (https://dwcode.pages.dev). Needs `npx wrangler login` on the Darkstardevx
# Cloudflare account. Pass --shots to recapture the app screenshots first.
set -euo pipefail
cd "$(dirname "$0")/.."
./tools/build-site.sh "$@"
npx -y wrangler@4 pages deploy site --project-name dwcode --branch main --commit-dirty=true
