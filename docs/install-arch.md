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
- Declarative packs validate but are not loaded at runtime.
- Git, workspace search, debugging, and executable extensions are not yet
  available.
