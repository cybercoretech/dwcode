#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
output="${root}/dist"
package_dir="${root}/packaging/arch"
version="$(awk '/^VERSION = / { print $3; exit }' "${root}/dwcode.pro")"
archive="${package_dir}/dwcode-${version}.tar"

if [[ "${1:-}" == "--output" ]]; then
  output="${2:?--output requires a directory}"
  shift 2
fi
if (( $# )); then
  printf 'Usage: %s [--output DIRECTORY]\n' "$0" >&2
  exit 2
fi

mkdir -p "${output}"
trap 'rm -f "${archive}"' EXIT
git -C "${root}" archive --format=tar --prefix="dwcode-${version}/" HEAD > "${archive}"

(
  cd "${package_dir}"
  makepkg --cleanbuild --force
)

packages=("${package_dir}"/dwcode-"${version}"-*.pkg.tar.zst)
if (( ${#packages[@]} != 1 )) || [[ ! -f "${packages[0]}" ]]; then
  printf 'Expected one package for DWCode %s, found %s.\n' \
    "${version}" "${#packages[@]}" >&2
  exit 1
fi

cp "${packages[0]}" "${output}/"
sha256sum "${packages[0]}" > "${output}/$(basename "${packages[0]}").sha256"
