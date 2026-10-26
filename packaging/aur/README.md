# 📦 AUR packages for DWCode

Two Arch User Repository packages live here, both maintained out of the
upstream repo so a release never means editing files by hand:

| Package | Type | Who wants it |
| --- | --- | --- |
| [`dwcode`](dwcode/) | source build (`makepkg`) | people who rebuild from the tag |
| [`dwcode-bin`](dwcode-bin/) | prebuilt `usr/` tarball | people who just want the editor installed |

`dwcode-bin` carries `provides=('dwcode')` and `conflicts=('dwcode')` — never
`replaces` — so exactly one of the two can be installed and a dependant asking
for `dwcode` resolves to whichever is present, without a binary install ever
silently taking over somebody's source build.

## Files

```
packaging/aur/
├── README.md              ← this file
├── dwcode/PKGBUILD        ← source package (tag tarball, npm + qmake6)
├── dwcode/.SRCINFO        ← generated: makepkg --printsrcinfo
├── dwcode-bin/PKGBUILD    ← prebuilt package (release asset)
├── dwcode-bin/.SRCINFO    ← generated
└── make-bin-tarball.sh    ← repacks an Arch package into the -bin asset
```

The `.SRCINFO` files are generated, never hand-edited. Every field they
contain is derived from the matching `PKGBUILD`.

## Releasing

`tools/aur-release.sh` does the whole job: bump `pkgver`, recompute both
`sha256sums`, regenerate both `.SRCINFO`, and push to
`ssh://aur@aur.archlinux.org/<pkg>.git`.

```sh
# 1. tag and push (the source tarball is derived from it)
git tag -a v0.2.0 -m 'DWCode 0.2.0' && git push origin v0.2.0

# 2. build the prebuilt asset for dwcode-bin
./packaging/arch/build-package.sh          # Arch package
packaging/aur/make-bin-tarball.sh          # → dist/dwcode-0.2.0-x86_64.tar.gz

# 3. attach it to the release
gh release create v0.2.0 dist/dwcode-0.2.0-x86_64.tar.gz

# 4. cut the AUR release
tools/aur-release.sh                       # checks the tag, checksums, pushes

# 5. commit the regenerated PKGBUILD/.SRCINFO back to the repo
git add packaging/aur && git commit -m 'AUR: 0.2.0-1'
```

Useful flags (`tools/aur-release.sh --help` for the full list):

* `--dry-run` — do everything except the SSH push; prints what would go.
* `--skip-bin` — publish only the source package (useful before the first
  binary asset exists).
* `--bin-url` / `--bin-sha256` — point at an asset built elsewhere, or hand
  it the checksum.
* `--allow-missing-tag` — offline experiments only; `sha256sums` stays
  `SKIP`, and the script says so loudly. **Never publish with SKIP**: an
  unverifiable package is exactly what the AUR is not for.

While `dwcode.pro` still says `0.1.0` and no `v0.1.0` tag exists on GitHub,
step 4 needs `--allow-missing-tag` (or `--dry-run`) to run at all.

## First-time SSH setup

```sh
ssh-keygen -t ed25519 -C 'aur@darkstardevx'
ssh-copy-id aur@aur.archlinux.org      # or paste the key into your AUR profile
ssh aur@aur.archlinux.org              # expect: "Welcome to the AUR!"
```

Both packages must already exist on the AUR before the script can clone them —
submit them once through <https://aur.archlinux.org/> (Account → Submit a
package), pointing at this repo's `packaging/aur/<name>/` path. After that the
script pulls, copies `PKGBUILD` + `.SRCINFO`, commits `<pkgver>-<pkgrel>` and
pushes.

## Building the `-bin` asset

`make-bin-tarball.sh` lifts the `package()` tree out of an Arch-built package
and re-tars it as a plain `usr/` directory (AUR packages install files; they
cannot contain another `.pkg.tar.zst`). The output is byte-reproducible —
sorted entries, fixed mtime/owner, `gzip -n` — so the checksum recorded in the
PKGBUILD is stable across machines. Verified: 109 entries, `usr/bin/dwcode`
present, identical sha256 on repeat runs.

The binary is always built **on Arch, against Arch's `qt6-webengine`** — never
a bundle of foreign Qt libraries, which would break silently at runtime.

## Where things are *not*

* `packaging/arch/PKGBUILD` — the **local** package build used by
  `build-package.sh` on this machine. Same layout, no AUR metadata, not pushed
  anywhere.
* The AUR itself — this directory is the source of truth; the AUR copies are
  the output of `tools/aur-release.sh`.
