# 🤝 DWCode agent handoff

**Last updated:** 2026-10-25  
**Repository:** [`cybercoretech/dwcode`](https://github.com/cybercoretech/dwcode)  
**Current default branch:** `main`

## ✅ Completed milestones

- 🐧 **Arch / Omarchy alpha:** DWCode has a reproducible Arch package, desktop
  launcher, Railshift icon, bundled Monaco assets, clean uninstall path, CI,
  and a tag-driven release-artifact workflow.
- 🧭 **Codium-inspired application chrome:** `main` includes a compact,
  functional File/Edit/Selection/View/Run/Terminal/Settings/Help menu bar,
  workspace controls, and Cybercore-accented document tabs.
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
  Later external versions still prompt, while duplicate notifications for the
  same version are suppressed. Landed in `907f93c`.

## 🧾 Recent implementation record

- `ef3b2fe feat(settings): add persistent preferences dialog` added searchable,
  persisted editor, workspace, terminal, appearance, and modal-editing
  preferences, including live Monaco application.
- `907f93c feat(workspace): add new files and change detection` added the first
  reliability slice: New File UI and API, recursive workspace watching,
  external-change/reload handling, and test coverage.

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

Follow this product sequence; the first two reliability items are complete.

1. 💾 **Finish reliability:** add crash recovery/session restore, then perform
   a real-display layout pass for Settings and the right-side Explorer.
2. 🔎 **Navigation:** implement `Ctrl+P` fuzzy Quick Open, then `Ctrl+Shift+F`
   workspace search backed by `rg` with clickable result locations.
3. 🌿 **Git:** add Explorer status decorations, a diff view, stage/unstage, and
   a focused commit flow. Build it on the watcher foundation above.
4. ▶️ **Tasks and terminal:** add named build/test/run tasks and problem
   matchers; replace the command runner with a PTY only when its interaction
   and lifecycle model is ready.
5. 🧩 **Data-only packs:** load validated theme, icon, snippet, and keymap packs
   with safe preview, enable/disable, and reset controls.
6. 🔐 **Extensions:** design the sandboxed extension host and capability model
   before executing third-party code. Keep extensions isolated from the Qt app,
   Monaco page, filesystem, terminal, and network behind explicit RPC-granted
   permissions; distribution/registry work comes last.

Separate release verification remains worthwhile: test-install the newest Arch
package locally and verify launcher, window icon, bundled assets, and uninstall
behavior.
