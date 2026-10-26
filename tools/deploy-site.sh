#!/usr/bin/env bash
# Builds site/ and uploads it to the Cloudflare Pages project "dwcode"
# (https://dwcode.pages.dev). Needs `npx wrangler login` on the Darkstardevx
# Cloudflare account (the OAuth scopes already include pages:write).
#   ./tools/deploy-site.sh            deploy using the wrangler login
#   ./tools/deploy-site.sh --shots    recapture the app screenshots first
#   ./tools/deploy-site.sh --use-token  deploy with $CLOUDFLARE_API_TOKEN instead
set -euo pipefail
cd "$(dirname "$0")/.."

use_token=0
args=()
for a in "$@"; do
  case "$a" in
    --use-token) use_token=1 ;;
    *) args+=("$a") ;;
  esac
done

./tools/build-site.sh ${args[@]+"${args[@]}"}

# Auth: wrangler prefers $CLOUDFLARE_API_TOKEN over `wrangler login`, and the
# token exported in this environment has no Pages permission — it dies with
# "Authentication error [code: 10000]" while the OAuth login works fine. So the
# token is dropped unless --use-token asked for it. The account id is supplied
# as a fallback because a token without Account Read cannot enumerate accounts
# ("Failed to automatically retrieve account IDs for the logged in user"); it is
# also what the zone API returns for cybercoretech.net (Darkstardevx).
export CLOUDFLARE_ACCOUNT_ID="${CLOUDFLARE_ACCOUNT_ID:-f770e07e62b976b12330421edcf2a7d9}"
[[ "$use_token" == 1 ]] || unset CLOUDFLARE_API_TOKEN

# Run wrangler outside the repo: in the repo root its Workers autoconfig
# detects Vite and rewrites package.json and vite.config.js.
site="$PWD/site"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
(cd "$tmp" && npx -y wrangler@4 pages deploy "$site" --project-name dwcode --branch main --commit-dirty=true)
