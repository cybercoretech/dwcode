# 🤝 DWCode agent handoff

**Last updated:** 2026-10-25  
**Repository:** [`cybercoretech/dwcode`](https://github.com/cybercoretech/dwcode)  
**Current default branch:** `main`

## ✅ Completed milestones

- 🐧 **Arch / Omarchy alpha:** DWCode has a reproducible Arch package, desktop
  launcher, Railshift icon, bundled Monaco assets, clean uninstall path, CI,
  and a tag-driven release-artifact workflow.
- 🧭 **Codium-inspired application chrome:** `main` includes a compact,
  functional File/Edit/Selection/View/Run/Terminal/Help menu bar, workspace
  controls, and Cybercore-accented document tabs.
- 📂 **Explorer and window identity:** the Explorer header has **Open Folder**
  and refresh controls. Railshift is set explicitly on the native Qt window as
  well as the desktop entry, so the compositor receives DWCode's app icon.
- 🎨 **Theme integration:** the shell and Monaco editor remain bound to the
  persisted 84-theme catalog. Do not hard-code interface colors outside the
  existing theme tokens.

## 🧪 Validation baseline

GitHub Actions on `main` currently validates:

- Monaco build and production dependency audit
- desktop-entry validation
- native Qt build
- backend tests
- offscreen Qt WebEngine UI tests

Run the focused local UI suite:

```sh
mkdir -p build/ui-tests
cd build/ui-tests
qmake6 ../../tests/ui.pro
make -j"$(nproc)"
QTWEBENGINE_DISABLE_SANDBOX=1 ./ui-tests
```

The current UI suite has 10 passing checks, including Explorer folder-control
availability and the native window icon.

## 💻 Local installation state

The packaged application is source-independent, but it is **not automatically
reinstalled after each merge**. Build the current `main` package from a clean
checkout, then install it:

```sh
git pull --ff-only
./packaging/arch/build-package.sh
sudo pacman -U dist/dwcode-0.1.0-1-x86_64.pkg.tar.zst
```

Launch it with `dwcode` or from the Omarchy application launcher. Do not alter
an unrelated dirty checkout without explicit reconciliation.

## ⚠️ Current product boundaries

- The terminal is a command runner, not a PTY.
- File watching, New File, crash recovery, Git, workspace search, debugging,
  runtime pack loading, and executable extensions are not implemented.
- TypeScript is bundled for LSP. C/C++, Rust, and Python support require their
  language servers on `PATH`.
- Declarative packs validate but are not loaded by the application yet.

## 🛠️ Next recommended work

1. 📦 Test-install the newest package locally and verify launcher, window icon,
   bundled assets, and uninstall behavior.
2. 📝 Add New File, file watching, and crash recovery before larger integrations.
3. 🔎 Add workspace search and Git status/actions.
4. 🧩 Connect declarative theme, icon, snippet, and keymap packs to the live UI
   with safe preview and reset controls.
5. 🔐 Design the extension-host security model before allowing executable
   third-party code.
