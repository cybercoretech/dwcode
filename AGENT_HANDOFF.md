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
- 💾 **Crash recovery and session restore:** open tabs, tab order, the active
  file, and unsaved buffers are journaled to a versioned `session.json` plus
  per-file buffers beside the QSettings store, and restored on the next
  launch. Restored dirty documents keep their journaled disk hash, so saving
  over a file that changed while DWCode was closed is refused and reported as
  an external change. **Keep Changes** was added to the close dialog, plus a
  `files/restoreSession` preference. Landed in `6389275`.
- 🖥️ **Real-display layout pass:** verified Settings, the command palette,
  and the right-side Explorer on Wayland at scale 1. Fixed two defects
  surfaced by the pass: input placeholders rendered near-black
  (`palette.placeholderText` was unset) and the settings search field had no
  background or focus ring. Captures in
  `~/.cache/omarchy/tmp/opencode/dwcode-layout/`; the `layout()` slot in
  `tests/shots.cpp` reproduces them.

## 🧾 Recent implementation record

- `6389275 feat(session): restore unsaved work and tabs across restarts` added
  the session journal and restore path, the Keep Changes close flow, the
  restore preference, a `saveAs` duplicate-tab fix, and the real-display
  layout fixes.
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

The current UI suite has 13 passing checks, including Explorer folder-control
availability, the native window icon, the Settings dialog, New File, and
`unsavedWorkSurvivesRestart` (edit → close → relaunch → buffer restored).

### Current validation

- `npm ci` and `npm run build` completed successfully. (`node_modules/` is
  ignored and was installed locally only.) `npm run validate:packs` passes:
  3 themes, 7 icon assets, 3 snippet groups.
- The full backend suite passed **9/9 checks**, including `sessionRestore`
  (tab order, active file, unsaved buffer, external-change refusal, deleted
  file retention, discard stays discarded).
- The full offscreen Qt WebEngine UI suite passed **13/13 checks**, including
  `settingsDialogAndEditorPreferences` and `unsavedWorkSurvivesRestart`. In
  this environment it runs outside the command sandbox so Qt WebEngine can
  create its loopback server and browser subprocess. Vulkan fallback warnings
  are expected in offscreen mode.
- **Real-display layout pass completed** on Wayland at `QT_SCALE_FACTOR=1`.
  Settings, the settings search (unfocused and filtered), the command palette,
  and the right-side Explorer were captured and reviewed; the two defects it
  surfaced are fixed. Reproduce with:

```sh
mkdir -p build/site-shots && cd build/site-shots
qmake6 ../../tests/shots.pro && make -j"$(nproc)"
DWCODE_UI_PLATFORM=wayland QT_SCALE_FACTOR=1 QTWEBENGINE_DISABLE_SANDBOX=1 \
  ./site-shots <output-dir>
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
- Workspace: delayed autosave, explorer glob exclusions, and **Reopen files
  from the last session** (on by default).
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
- Git, workspace search, debugging, runtime pack loading, and executable
  extensions are not implemented. Session restore covers editor documents only
  (not open panels, split state, or terminal scrollback).
- TypeScript is bundled for LSP. C/C++, Rust, and Python support require their
  language servers on `PATH`.
- Declarative packs validate but are not loaded by the application yet.

## 🛠️ Next recommended work

Follow this product sequence; the reliability items are complete.

1. ✅ **Reliability (done):** crash recovery/session restore and the
   real-display layout pass landed in `6389275`.
2. 🔎 **Navigation:** implement `Ctrl+P` fuzzy Quick Open, then `Ctrl+Shift+F`
   workspace search backed by `rg` with clickable result locations.
3. 🌿 **Git:** add Explorer status decorations, a diff view, stage/unstage, and
   a focused commit flow. Build it on the watcher foundation above.
4. ▶️ **Tasks and terminal:** add named build/test/run tasks and problem
   matchers; replace the command runner with a PTY only when its interaction
   and lifecycle model is ready.
5. 🧩 **Data-only packs:** load validated theme, icon, snippet, and keymap packs
   with safe preview, enable/disable, and reset controls (blueprint phase M1).
6. 🔐 **Extensions:** follow [`EXTENSIONS_BLUEPRINT.md`](EXTENSIONS_BLUEPRINT.md)
   — M0 converts the closed command/menu/status/keybinding surfaces in
   `Main.qml` into registries, M1 loads declarative packs, M2 adds the
   out-of-process host with the versioned JSON-RPC handshake, and M3/M4 add the
   narrow executable API behind the capability broker. Keep extensions isolated
   from the Qt app, Monaco page, filesystem, terminal, and network behind
   explicit RPC-granted permissions; distribution/registry work is M5.

Separate release verification remains worthwhile: test-install the newest Arch
package locally and verify launcher, window icon, bundled assets, and uninstall
behavior.
