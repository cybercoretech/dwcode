# 🐧 DWCode Arch / Omarchy alpha

DWCode's alpha package installs the native editor, Railshift launcher icon,
desktop entry, and bundled Monaco web assets. The installed application does
not read editor assets from a source checkout.

## 📦 Build and install

Install the documented build requirements:

```sh
sudo pacman -S --needed base-devel git nodejs npm qt6-base qt6-declarative \
  qt6-webchannel qt6-webengine
```

Build a package from a clean DWCode checkout, then install it with pacman:

```sh
git clone https://github.com/cybercoretech/dwcode.git
cd dwcode
./packaging/arch/build-package.sh
sudo pacman -U dist/dwcode-0.1.0-1-x86_64.pkg.tar.zst
```

Launch **DWCode** from the Omarchy application launcher or run:

```sh
dwcode
dwcode --workspace ~/path/to/project
```

## 📮 Arch User Repository (AUR)

Besides building locally, DWCode ships two AUR package definitions maintained
inside this repository:

- **`dwcode`** — builds from the GitHub tag with `makepkg` (`npm ci`,
  `npm run build`, `qmake6`, `make`), then runs `npm run validate:packs` and
  the backend test suite in `check()`. For people who want the source build.
- **`dwcode-bin`** — downloads a prebuilt `usr/` tarball from the GitHub
  release and unpacks it. `provides=('dwcode')` and `conflicts=('dwcode')`
  (never `replaces`) keep the two mutually exclusive while letting either
  satisfy a `dwcode` dependency. For people who just want the editor.

Both live under `packaging/aur/` with a generated `.SRCINFO`, and both are
updated by a single command:

```sh
tools/aur-release.sh --dry-run    # verify: tag check, checksums, .SRCINFO, pushes
tools/aur-release.sh              # publish to ssh://aur@aur.archlinux.org/<pkg>.git
```

`tools/aur-release.sh` refuses to push while any `sha256sums` is `SKIP`, so
nothing unverifiable can reach the AUR. Once published, install with an AUR
helper:

```sh
yay -S dwcode          # source build
yay -S dwcode-bin      # prebuilt
```

First-time SSH key registration, the `-bin` asset recipe
(`packaging/aur/make-bin-tarball.sh`), and the full release checklist are in
[`packaging/aur/README.md`](../packaging/aur/README.md).

## 🔄 Update and uninstall

Build a new package from the revision you want to use, then replace the
installed package:

```sh
sudo pacman -U dist/dwcode-0.1.0-1-x86_64.pkg.tar.zst
```

Remove DWCode and its package-managed files:

```sh
sudo pacman -Rns dwcode
```

## ⚠️ Alpha limitations

- The terminal is a command runner, not a full PTY.
- Session restore covers editor documents; open panels and terminal scrollback
  are not restored.
- Language servers other than bundled TypeScript must already be installed.
- Declarative packs load in-process as data (themes and snippets, toggleable
  in Settings → EXTENSIONS); executable extensions are not available.
- Git, workspace search, debugging, and executable extensions are not yet
  available.
