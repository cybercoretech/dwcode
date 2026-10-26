#!/usr/bin/env bash
# tools/aur-release.sh — cut an AUR release for DWCode.
#
# Bumps pkgver in both AUR PKGBUILDs (source `dwcode` and prebuilt
# `dwcode-bin`), recomputes both sha256 checksums from the real artifacts,
# regenerates both .SRCINFO files, and pushes each package to its own AUR git
# repository over SSH.
#
#   tools/aur-release.sh                  # release whatever is in dwcode.pro
#   tools/aur-release.sh --dry-run        # everything except the AUR push
#   tools/aur-release.sh --pkgver 0.2.0 --pkgrel 2
#   tools/aur-release.sh --bin-url https://…/dwcode-0.2.0-x86_64.tar.gz
#   tools/aur-release.sh --skip-bin       # publish only the source package
#
# Preconditions
#   * tag v<pkgver> exists on GitHub (the source tarball is derived from it)
#   * for -bin: the release asset exists, or --bin-url points at one:
#       ./packaging/arch/build-package.sh      # builds the Arch package
#       packaging/aur/make-bin-tarball.sh      # repacks it to a usr/ tarball
#       gh release create v<pkgver> dist/dwcode-<pkgver>-x86_64.tar.gz
#   * `ssh aur@aur.archlinux.org` works with your AUR account key
#   * makepkg (pacman) is installed — it generates .SRCINFO
#
# Options
#   --pkgver VERSION    version to release (default: VERSION in dwcode.pro)
#   --pkgrel N          pkgrel to publish (default: 1, or incremented when the
#                       current PKGBUILD already carries this pkgver)
#   --bin-url URL       prebuilt tarball URL (default: the GitHub release asset)
#   --bin-sha256 SHA    use this checksum instead of downloading the asset
#   --skip-bin          update only the source package
#   --aur-root DIR      where the AUR checkouts live (default: build/aur)
#   --allow-missing-tag do not require the GitHub tag (offline experiments)
#   --dry-run           compute and write everything, print the pushes, stop
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

repo_url="https://github.com/cybercoretech/dwcode"
aur_base="ssh://aur@aur.archlinux.org"

pkgver="$(awk '/^VERSION = / { print $3; exit }' "$root/dwcode.pro")"
pkgrel=""
bin_url=""
bin_sha=""
skip_bin=0
aur_root="$root/build/aur"
allow_missing_tag=0
dry_run=0

while (( $# )); do
  case "$1" in
    --pkgver) pkgver="${2:?--pkgver needs a version}"; shift 2 ;;
    --pkgrel) pkgrel="${2:?--pkgrel needs a number}"; shift 2 ;;
    --bin-url) bin_url="${2:?--bin-url needs a url}"; shift 2 ;;
    --bin-sha256) bin_sha="${2:?--bin-sha256 needs a hash}"; shift 2 ;;
    --skip-bin) skip_bin=1; shift ;;
    --aur-root) aur_root="${2:?--aur-root needs a directory}"; shift 2 ;;
    --allow-missing-tag) allow_missing_tag=1; shift ;;
    --dry-run) dry_run=1; shift ;;
    -h|--help) awk 'NR > 1 && /^#/ { sub(/^# ?/, ""); print; next } NR > 1 { exit }' "$0"; exit 0 ;;
    *) printf 'unknown argument: %s (see --help)\n' "$1" >&2; exit 2 ;;
  esac
done

[[ "$pkgver" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || { printf 'pkgver %q is not X.Y.Z\n' "$pkgver" >&2; exit 2; }
tag="v$pkgver"
src_pb="$root/packaging/aur/dwcode/PKGBUILD"
bin_pb="$root/packaging/aur/dwcode-bin/PKGBUILD"
pbs=("$src_pb")
if (( ! skip_bin )); then pbs+=("$bin_pb"); fi

[[ -f "$src_pb" ]] || { printf 'missing %s\n' "$src_pb" >&2; exit 1; }
(( skip_bin )) || [[ -f "$bin_pb" ]] || { printf 'missing %s\n' "$bin_pb" >&2; exit 1; }
command -v makepkg >/dev/null || { printf 'makepkg not found — install pacman.\n' >&2; exit 1; }
command -v curl >/dev/null || { printf 'curl not found.\n' >&2; exit 1; }

step() { printf '\n▸ %s\n' "$*"; }
info() { printf '  %s\n' "$*"; }
die()  { printf '\n✗ %s\n' "$*" >&2; exit 1; }

# ---------------------------------------------------------------- 1. tag
step "checking the release tag"
if git ls-remote --exit-code --tags "$repo_url" "refs/tags/$tag" >/dev/null 2>&1; then
  info "$tag exists on GitHub"
elif (( allow_missing_tag )); then
  info "$tag is missing — continuing because --allow-missing-tag (checksums stay SKIP)"
else
  die "$tag does not exist yet.
  The source tarball is derived from it, so tag and push first:
    git tag -a $tag -m 'DWCode $pkgver' && git push origin $tag
  (or pass --allow-missing-tag for an offline experiment)"
fi

# ---------------------------------------------------------------- 2. artifacts
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

src_sha="SKIP"
if (( ! allow_missing_tag )); then
  step "checksumming the source tarball"
  src_tar="$work/dwcode-$pkgver.tar.gz"
  curl -fsSL --retry 3 --retry-delay 2 -o "$src_tar" \
    "$repo_url/archive/refs/tags/$tag.tar.gz" \
    || die "could not download $repo_url/archive/refs/tags/$tag.tar.gz"
  # Guard against a tag that does not match dwcode.pro: the tarball has to
  # declare the version we are about to publish.
  tar -xzOf "$src_tar" "dwcode-$pkgver/dwcode.pro" 2>/dev/null \
    | awk '/^VERSION = / { print $3; exit }' | grep -qx "$pkgver" \
    || die "the $tag tarball does not declare VERSION = $pkgver in dwcode.pro"
  src_sha="$(sha256sum "$src_tar" | awk '{ print $1 }')"
  info "source sha256  $src_sha"
fi

bin_sha_final=""
if (( ! skip_bin )); then
  [[ -n "$bin_url" ]] || bin_url="$repo_url/releases/download/$tag/dwcode-$pkgver-x86_64.tar.gz"
  if [[ -n "$bin_sha" ]]; then
    bin_sha_final="$bin_sha"
    info "prebuilt sha256 (given)  $bin_sha_final"
  else
    step "checksumming the prebuilt tarball"
    bin_tar="$work/dwcode-$pkgver-x86_64.tar.gz"
    curl -fsSL --retry 3 --retry-delay 2 -o "$bin_tar" "$bin_url" || die "no prebuilt asset at
  $bin_url
Build and upload one first:
  ./packaging/arch/build-package.sh
  packaging/aur/make-bin-tarball.sh
  gh release create $tag dist/dwcode-$pkgver-x86_64.tar.gz
or pass --bin-url / --bin-sha256, or --skip-bin to publish only the source package."
    tar -tzf "$bin_tar" | grep -q '^usr/bin/dwcode$' \
      || die "the prebuilt tarball has no usr/bin/dwcode — it was not produced by packaging/aur/make-bin-tarball.sh"
    bin_sha_final="$(sha256sum "$bin_tar" | awk '{ print $1 }')"
    info "prebuilt sha256  $bin_sha_final"
    info "asset URL        $bin_url"
  fi
fi

# ---------------------------------------------------------------- 3. pkgrel
if [[ -z "$pkgrel" ]]; then
  # Read back tolerantly: an earlier run may have quoted the value.
  read_field() { awk -F= -v k="$2" 'index($0, k "=") == 1 { v = substr($0, length(k) + 2); gsub(/\x27/, "", v); print v; exit }' "$1"; }
  current="$(read_field "$src_pb" pkgver)"
  if [[ "$current" == "$pkgver" ]]; then
    published="$(read_field "$src_pb" pkgrel)"
    [[ "$published" =~ ^[0-9]+$ ]] || published=1
    pkgrel=$(( published + 1 ))
    info "same version as the committed PKGBUILD — bumping pkgrel to $pkgrel"
  else
    pkgrel=1
  fi
fi

# ---------------------------------------------------------------- 4. rewrite
# Replace a single `key=value` assignment anywhere in the PKGBUILD. Used only
# for pkgver and pkgrel, which are bare scalars by convention — quote handling
# for checksums lives in set_sha256() below.
set_field() {
  local file="$1" key="$2" value="$3" tmp="$1.tmp"
  awk -v k="$key" -v v="$value" '
    BEGIN { done = 0 }
    index($0, k "=") == 1 {
      if (!done) { print k "=" v; done = 1 }
      next
    }
    { print }
    END { if (!done) { print k "=" v } }
  ' "$file" > "$tmp" && mv "$tmp" "$file"
}

# sha256sums must stay an array literal — a plain scalar makes makepkg bail
# out with "sha256sums should be an array".
set_sha256() {
  local file="$1" value="$2" tmp="$1.tmp"
  awk -v v="$value" '
    BEGIN { done = 0 }
    /^sha256sums=/ {
      if (!done) { print "sha256sums=(\x27" v "\x27)"; done = 1 }
      next
    }
    { print }
    END { if (!done) { print "sha256sums=(\x27" v "\x27)" } }
  ' "$file" > "$tmp" && mv "$tmp" "$file"
}

for pb in "${pbs[@]}"; do
  name="$(basename "$(dirname "$pb")")"
  step "updating packaging/aur/$name/PKGBUILD"
  set_field "$pb" pkgver "$pkgver"
  set_field "$pb" pkgrel "$pkgrel"
  if [[ "$name" == "dwcode" ]]; then set_sha256 "$pb" "$src_sha"; else set_sha256 "$pb" "$bin_sha_final"; fi
  info "pkgver=$pkgver  pkgrel=$pkgrel  sha256sums=$(awk -F"'" '/^sha256sums=/ { print $2; exit }' "$pb")"
  bash -n "$pb" || die "$pb has a syntax error"
  if grep -q "^sha256sums=('SKIP')" "$pb"; then
    (( allow_missing_tag )) || die "$pb still has SKIP — refusing to publish an unverifiable package"
    info "warning: still carries SKIP (allowed by --allow-missing-tag)"
  fi
done

# ---------------------------------------------------------------- 5. .SRCINFO
step "regenerating .SRCINFO"
for pb in "${pbs[@]}"; do
  dir="$(dirname "$pb")"
  (cd "$dir" && makepkg --printsrcinfo > .SRCINFO) || die "makepkg --printsrcinfo failed in $dir"
  info "$(basename "$dir")/.SRCINFO  ($(grep -c . "$dir/.SRCINFO") lines)"
done

# ---------------------------------------------------------------- 6. push
push_pkg() {
  local name="$1" dir="$root/packaging/aur/$1" dst="$aur_root/$1"
  if (( dry_run )); then
    info "[dry-run] $name: copy {PKGBUILD,.SRCINFO} -> $dst, commit '$pkgver-$pkgrel', push to $aur_base/$name.git"
    return 0
  fi
  if [[ ! -d "$dst/.git" ]]; then
    mkdir -p "$aur_root"
    git clone --quiet "$aur_base/$name.git" "$dst" \
      || die "could not clone $aur_base/$name.git
  The AUR package has to exist before it can be updated. Submit it once through
  the AUR web interface (https://aur.archlinux.org/) or push the first commit
  to the freshly created empty repository, then re-run this script."
  else
    (cd "$dst" && git pull --ff-only --quiet) \
      || die "$dst has local commits that conflict with the AUR — resolve them by hand"
  fi
  cp "$dir/PKGBUILD" "$dir/.SRCINFO" "$dst/"
  if [[ -z "$(cd "$dst" && git status --porcelain)" ]]; then
    info "$name: already at $pkgver-$pkgrel, nothing to push"
    return 0
  fi
  (cd "$dst" && git add PKGBUILD .SRCINFO \
    && git commit --quiet -m "$pkgver-$pkgrel" \
    && git push --quiet origin HEAD)
  info "$name: pushed $pkgver-$pkgrel to $aur_base/$name.git"
}

step "AUR checkouts"
if (( dry_run )); then
  info "dry run — skipping the SSH preflight and the pushes"
else
  ssh_out="$(ssh -o BatchMode=yes -o ConnectTimeout=10 aur@aur.archlinux.org 2>&1 || true)"
  if [[ "$ssh_out" != *"Welcome to the AUR"* ]]; then
    die "cannot reach the AUR over SSH:
  $(printf '%s' "$ssh_out" | tr -d '\r' | head -2 | tail -1)
Register this public key with your AUR account
(https://aur.archlinux.org/account/darkstardevx/), then re-run:
    ssh-keygen -t ed25519 -C 'aur@darkstardevx'
    ssh-copy-id aur@aur.archlinux.org
Use --dry-run to practise without pushing."
  fi
  info "$(printf '%s' "$ssh_out" | tr -d '\r' | head -1)"
fi

for name in dwcode dwcode-bin; do
  if [[ "$name" == "dwcode-bin" ]] && (( skip_bin )); then
    info "skipping dwcode-bin (--skip-bin)"
    continue
  fi
  push_pkg "$name"
done

# ---------------------------------------------------------------- 7. summary
step "done"
printf '  %-11s %-14s %s\n' package pkgver-pkgrel checksum
printf '  %-11s %-14s %s\n' dwcode "$pkgver-$pkgrel" "$src_sha"
if (( ! skip_bin )); then
  printf '  %-11s %-14s %s\n' dwcode-bin "$pkgver-$pkgrel" "$bin_sha_final"
fi
if (( dry_run )); then
  printf '\n  dry run — nothing was pushed. Re-run without --dry-run to publish.\n'
fi
printf '\n  commit the regenerated files to the repo:\n'
printf '    git add packaging/aur && git commit -m "AUR: %s-%s"\n' "$pkgver" "$pkgrel"
