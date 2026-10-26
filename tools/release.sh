#!/usr/bin/env bash
# tools/release.sh — cut a DWCode release: gate, tag, build the assets, publish.
#
#   tools/release.sh --dry-run          # show every step, change nothing
#   tools/release.sh                    # tag + release + assets
#   tools/release.sh --skip-build       # skip the Arch package / -bin asset
#   tools/release.sh --with-aur         # finish with tools/aur-release.sh
#
# Phase 0 of docs/report-m5-api-roadmap.md, in one command. In order:
#
#   1. refuse to release a dirty or stale tree
#   2. run the offline gates (packs, catalog+index, grammar)
#   3. rebuild dist/packs/*.dwpack so the artifact behind the index exists
#   4. tag v<version> (from dwcode.pro) and push main + the tag
#   5. build the Arch package and repack the deterministic -bin tarball
#   6. `gh release create` with the -bin tarball, the pack artifacts, and a
#      snapshot of index.json + index.json.sha256 — the release then carries
#      the exact index that authorized installs made against this version
#   7. optionally hand over to tools/aur-release.sh
#
# The tag is never moved and a release is never overwritten: publishing is
# append-only, so a checksum recorded yesterday still resolves today.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

version="$(awk '/^VERSION = / { print $3; exit }' "$root/dwcode.pro")"
tag="v${version}"
repo="https://github.com/cybercoretech/dwcode"
skip_build=0
with_aur=0
dry_run=0

while (( $# )); do
  case "$1" in
    --skip-build) skip_build=1; shift ;;
    --with-aur)   with_aur=1; shift ;;
    --dry-run)    dry_run=1; shift ;;
    -h|--help)    awk 'NR > 1 && /^#/ { sub(/^# ?/, ""); print; next } NR > 1 { exit }' "$0"; exit 0 ;;
    *) printf 'unknown argument: %s (see --help)\n' "$1" >&2; exit 2 ;;
  esac
done

step() { printf '\n▸ %s\n' "$*"; }
info() { printf '  %s\n' "$*"; }
die()  { printf '\n✗ %s\n' "$*" >&2; exit 1; }

[[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || die "VERSION in dwcode.pro is not X.Y.Z: $version"
for tool in git node npm gh; do
  command -v "$tool" >/dev/null || die "$tool is required"
done

# ---------------------------------------------------------------- 1. tree
step "checking the tree"
[[ -z "$(git status --porcelain | grep -v '^?? site/screenshots/')" ]] \
  || die "the working tree has uncommitted changes — commit or stash them first
so the tag describes exactly what was tested:
$(git status --porcelain | grep -v '^?? site/screenshots/')"
[[ "$(git branch --show-current)" == "main" ]] || die "releases are cut from main, not $(git branch --show-current)"
git fetch --quiet origin main || die "cannot reach $repo"
# `rev-list --count` prints 0 when level, and `[[ -z "0" ]]` would be false —
# compare the number, not the emptiness of the string.
ahead="$(git rev-list --count origin/main..HEAD)"
[[ "$ahead" == "0" ]] || die "HEAD is $ahead commit(s) ahead of origin/main — push first"
git ls-remote --exit-code --tags "$repo" "refs/tags/$tag" >/dev/null 2>&1 \
  && die "$tag already exists on GitHub. Bump VERSION in dwcode.pro (and package.json) to release again — a tag is never moved."
info "main == origin/main at $(git rev-parse --short HEAD), no $tag yet"

# ---------------------------------------------------------------- 2. gates
step "offline gates"
if (( dry_run )); then
  info "[dry-run] npm run validate:packs && npm run validate:catalog && npm run grammar:test"
else
  npm run validate:packs >/dev/null    || die "validate:packs failed"
  info "validate:packs OK"
  npm run validate:catalog >/dev/null  || die "validate:catalog failed (data, HTML, index, checksums)"
  info "validate:catalog OK — page, index and artifact checksums agree"
  npm run grammar:test >/dev/null      || die "grammar:test failed"
  info "grammar:test OK"
fi

# ---------------------------------------------------------------- 3. packs
step "pack artifacts"
if (( dry_run )); then
  info "[dry-run] node tools/build-pack.mjs"
else
  node tools/build-pack.mjs
fi

# ---------------------------------------------------------------- 4. tag
step "tag"
info "git tag -a $tag -m 'DWCode $version' && git push origin main $tag"
if (( ! dry_run )); then
  git tag -a "$tag" -m "DWCode $version"
  git push --quiet origin main "$tag"
  info "pushed main and $tag"
fi

# ---------------------------------------------------------------- 5. assets
bin_asset=""
if (( skip_build )); then
  step "assets"
  info "skipped (--skip-build): no -bin tarball in this release"
elif (( dry_run )); then
  step "assets"
  info "[dry-run] ./packaging/arch/build-package.sh && packaging/aur/make-bin-tarball.sh"
else
  step "building the Arch package (makepkg: npm ci + vite + qmake6 + make)"
  ./packaging/arch/build-package.sh
  step "repacking the -bin asset"
  bin_asset="$(packaging/aur/make-bin-tarball.sh | tail -2 | head -1)"
  info "$bin_asset"
fi

# ---------------------------------------------------------------- 6. release
assets=()
[[ -n "$bin_asset" && -f "$bin_asset" ]] && assets+=("$bin_asset")
for dwpack in dist/packs/*.dwpack; do
  [[ -f "$dwpack" ]] && assets+=("$dwpack")
done
assets+=(site/data/catalog/index.json site/data/catalog/index.json.sha256)

notes="$(cat <<EOF
DWCode ${version}

## Install

- \`dwcode-${version}-x86_64.tar.gz\` — prebuilt Arch payload (\`usr/\`), what the \`dwcode-bin\` AUR package installs. Built on Arch against Arch's own qt6-webengine.
- Source tarball: the tag itself.

## Extension index

- \`index.json\` + \`index.json.sha256\` — snapshot of the install contract served at \`https://dwcode.pages.dev/data/catalog/index.json\`. Checksums recorded here are the ones installs made against this version were verified with. Format: \`docs/PACK_INDEX.md\`.
EOF
)"
[[ -n "$bin_asset" ]] || notes+=$'\n- Note: this release has no prebuilt -bin asset (`--skip-build`).'

step "release"
info "gh release create $tag ${assets[*]:-}"
if (( dry_run )); then
  info "[dry-run] nothing published"
else
  [[ -f site/data/catalog/index.json ]] || die "site/data/catalog/index.json missing — run npm run catalog:index"
  if git ls-remote --exit-code --tags "$repo" "refs/tags/$tag" >/dev/null 2>&1 \
     && gh release view "$tag" --repo cybercoretech/dwcode >/dev/null 2>&1; then
    die "$tag already has a release"
  fi
  gh release create "$tag" "${assets[@]}" \
    --repo cybercoretech/dwcode \
    --title "DWCode ${version}" \
    --notes "$notes"
  info "published: $repo/releases/tag/$tag"
fi

# ---------------------------------------------------------------- 7. AUR
if (( with_aur )); then
  step "AUR"
  if (( dry_run )); then
    info "[dry-run] tools/aur-release.sh --dry-run"
    tools/aur-release.sh --dry-run || die "aur-release.sh failed"
  else
    tools/aur-release.sh
  fi
fi

step "done"
printf '  version   %s\n  tag       %s\n  assets    %s\n' "$version" "$tag" "${#assets[@]}"
(( dry_run )) && printf '\n  dry run — nothing was tagged, built or published.\n'
if (( ! with_aur )); then
  printf '\n  AUR not touched. When the SSH key is registered:\n    tools/aur-release.sh\n'
fi
