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
- ⚙️ **Settings:** the menubar has **Settings** between
  Terminal and Help, with **Preferences… (Ctrl+,)**. It opens a searchable,
  persistent settings dialog covering Appearance, Editor, Editing, Files &
  Workspace, Terminal, and Keyboard & Modal Editing.
- 🛡️ **Reliability foundation:** **New File… (Ctrl+N)** creates an unsaved,
  workspace-confined document. The Explorer watches workspace folders, and
  changed or deleted open files prompt to reload or preserve the editor copy.

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

The current UI suite has 12 passing checks, including Explorer folder-control
availability, the native window icon, the Settings dialog, and New File.

### Current validation

- `npm ci` and `npm run build` completed successfully. (`node_modules/` is
  ignored and was installed locally only.)
- The full backend suite passed **8/8 checks**.
- The full offscreen Qt WebEngine UI suite passed **12/12 checks**, including
  `settingsDialogAndEditorPreferences`. In this Codex environment it runs
  outside the command sandbox so Qt WebEngine can create its loopback server
  and browser subprocess. Vulkan fallback warnings are expected in offscreen
  mode.
- **Still perform a real-display layout pass** for the dialog and the
  right-sidebar option before calling the UI visually signed off:

```sh
DWCODE_UI_PLATFORM=wayland QTWEBENGINE_DISABLE_SANDBOX=1 \
  QTWEBENGINE_CHROMIUM_FLAGS=--no-sandbox ./build/ui-tests/ui-tests \
  settingsDialogAndEditorPreferences
```

### Settings behavior implemented

- Preferences use DWCode's existing `QSettings` store and have a reset action.
- Appearance: theme, shell scale/density, Explorer side, status bar.
- Monaco editor: font, size, ligatures, line height, wrapping, minimap, line
  numbers, rulers, whitespace, guides, current-line highlight, and smooth
  scrolling; settings apply live to both editor views.
- Editing: indentation, tab size, auto-indent/closing pairs, whitespace
  cleanup, final newline, and CRLF output on save. Save normalization reports
  a `didChange` to a connected language server before `didSave`.
- Workspace: delayed autosave and explorer glob exclusions.
- Terminal: shell, workspace/home working directory, font, scrollback,
  copy-on-select, clear-before-run, multiline environment additions, and
  confirmation for a deliberately narrow set of risky command patterns.
- Keyboard: persisted modal-mode default and normal-mode cursor shape.

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
- Crash recovery, Git, workspace search, debugging, runtime pack loading, and
  executable extensions are not implemented.
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
