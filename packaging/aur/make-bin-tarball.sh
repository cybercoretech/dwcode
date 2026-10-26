#!/usr/bin/env bash
# Repack an Arch package into the plain usr/ tarball the `dwcode-bin` AUR
# package downloads. AUR packages install files; they cannot contain another
# .pkg.tar.zst, so the package() tree is lifted out and re-tarred.
#
#   packaging/aur/make-bin-tarball.sh                    # newest package found
#   packaging/aur/make-bin-tarball.sh --pkg path/to/dwcode-0.1.0-1-x86_64.pkg.tar.zst
#   packaging/aur/make-bin-tarball.sh --pkgver 0.1.0 --output /tmp/release
#
# Produces: <output>/dwcode-<pkgver>-x86_64.tar.gz  (+ .sha256)
# The result is byte-reproducible for identical inputs (sorted entries, fixed
# mtime/owner, gzip -n), so the sha256 recorded in the PKGBUILD stays valid
# no matter which machine builds it.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
pkg=""
pkgver="$(awk '/^VERSION = / { print $3; exit }' "${root}/dwcode.pro")"
output="${root}/dist"

while (( $# )); do
  case "$1" in
    --pkg) pkg="${2:?--pkg needs a file}"; shift 2 ;;
    --pkgver) pkgver="${2:?--pkgver needs a version}"; shift 2 ;;
    --output) output="${2:?--output needs a directory}"; shift 2 ;;
    -h|--help) grep '^#' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) printf 'unknown argument: %s\n' "$1" >&2; exit 2 ;;
  esac
done

if [[ -z "$pkg" ]]; then
  # Newest package in the tree, ignoring the -debug variant.
  pkg="$(ls -t "${root}"/packaging/arch/dwcode-"${pkgver}"-*.pkg.tar.zst 2>/dev/null | grep -v -- '-debug-' | head -1 || true)"
fi
[[ -n "$pkg" && -f "$pkg" ]] || {
  printf 'No package found. Build one first:\n  ./packaging/arch/build-package.sh\nor pass --pkg FILE.\n' >&2
  exit 1
}

command -v bsdtar >/dev/null || { printf 'bsdtar is required (pacman -S libarchive).\n' >&2; exit 1; }
command -v gzip   >/dev/null || { printf 'gzip is required.\n' >&2; exit 1; }

mkdir -p "$output"
stage="$(mktemp -d)"
trap 'rm -rf "$stage"' EXIT

# Unpack the package, keep only the payload (drops .PKGINFO/.MTREE/.BUILDINFO).
bsdtar -xf "$pkg" -C "$stage"
[[ -d "$stage/usr" ]] || { printf '%s has no usr/ payload.\n' "$pkg" >&2; exit 1; }

name="dwcode-${pkgver}-x86_64.tar.gz"
out="${output}/${name}"
# --sort/--mtime/--owner make the archive deterministic; gzip -n drops the
# timestamp gzip would otherwise stamp into the header.
tar --sort=name --mtime='@0' --owner=0 --group=0 --numeric-owner \
    -C "$stage" -cf - usr | gzip -9 -n > "$out"

(
  cd "$output"
  sha256sum "$name" > "${name}.sha256"
)
printf '%s\n' "$out"
cat "${out}.sha256"
printf 'upload as: releases/download/v%s/%s\n' "$pkgver" "$name"
