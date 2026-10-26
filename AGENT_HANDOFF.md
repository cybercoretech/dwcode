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

The current UI suite has 14 passing checks, including Explorer folder-control
availability, the native window icon, the Settings dialog, the EXTENSIONS
section (`extensionsSectionManagesPacks`), New File, and
`unsavedWorkSurvivesRestart` (edit → close → relaunch → buffer restored).

### Current validation

- `npm ci` and `npm run build` completed successfully. (`node_modules/` is
  ignored and was installed locally only.) `npm run validate:packs` passes:
  3 themes, 7 icon assets, 3 snippet groups.
- Two further gates run locally and in CI (`.github/workflows/ci.yml`):
  `npm run catalog:scan` — the licence gate over every extension id in
  `FOSS_EXTENSION_REPOSITORY.md` (currently **206 checked · 179 catalog-eligible
  · 0 pending**, exit 0, with hand-verified upstream licences in
  `tools/openvsx-verified.json`) — and `npm run grammar:test`, the
  `tools/tm2monarch.mjs --self-test` suite (16 conversion cases, each definition
  also compiled by Monaco's own `monarchCompile.js`).
- `./tools/deploy-site.sh` publishes `site/` to dwcode.pages.dev. It drops a
  `$CLOUDFLARE_API_TOKEN` that lacks Pages permission (wrangler prefers env
  tokens over `wrangler login` and dies with `code: 10000`) unless `--use-token`
  is passed, and exports `CLOUDFLARE_ACCOUNT_ID` as a fallback.
- The full backend suite passed **10/10 checks**, including `packLoader` (pack
  fixtures from `tests/fixtures/packs/`, bad-manifest rejection, `../` escape
  rejection, semver range table) and `sessionRestore`
  (tab order, active file, unsaved buffer, external-change refusal, deleted
  file retention, discard stays discarded).
- The full offscreen Qt WebEngine UI suite passed **14/14 checks**, including
  `settingsDialogAndEditorPreferences`, `extensionsSectionManagesPacks` and
  `unsavedWorkSurvivesRestart`. In
  this environment it runs outside the command sandbox so Qt WebEngine can
  create its loopback server and browser subprocess. Vulkan fallback warnings
  are expected in offscreen mode.
- **Real-display layout pass completed** on Wayland at `QT_SCALE_FACTOR=1`.
  Settings, the settings search (unfocused and filtered), the extensions
  section, the command palette, and the right-side Explorer were captured and
  reviewed; the two defects it surfaced are fixed. Reproduce with:

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

### Extensions (M0 + M1) implemented

- `src/qml/CommandRegistry.qml`: `register` / `unregister` / `run`;
  built-ins self-register in `Component.onCompleted`, the palette reads it.
- `src/packloader.{h,cpp}` re-validates every pack with the same rules as
  `tools/validate-packs.mjs` (fail-closed `engines.dwcode` semver, path
  containment on disk and for `:/` resources), so a file that merely passed CI
  is never trusted by the app.
- `WorkspaceBackend`: `packs`, `packSnippets`, `packDirectory()`,
  `setPackEnabled()` (persisted at `settings/packs/<id>/enabled`),
  `rebuildThemes()`.
- **Theme catalog split:** `src/data/themes.json` holds the **81** built-in
  themes; the three `cybercore.dwcode-*` themes live only in
  `packs/dwcode-core/themes/`. The shipped total stays **84** — the backend
  merges built-ins first, and `node tools/theme-catalog.mjs` (wired into
  `tools/build-site.sh`) regenerates `site/data/themes.json` with the merge
  for the site gallery. Do not add the pack themes back to `themes.json`.

Two traps found the hard way:

- **Never put a `R"json(...)"` raw string containing `"#RRGGBB` inside a
  moc-parsed `.cpp`** — Qt's moc silently emits "No relevant classes found"
  and the link fails with `undefined reference to vtable`. JSON fixtures live
  in `tests/fixtures/packs/` and are copied at runtime with `QFINDTESTDATA`.
- Toggling a pack rebuilds the Repeater, so its delegates are recreated; a UI
  test must re-locate `packToggle-<id>` after every toggle or it dereferences
  a deleted item.

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
- Git, workspace search, debugging, and executable extensions are not
  implemented. Declarative packs *are* loaded in-process (themes, snippets,
  enable/disable in Settings → EXTENSIONS); icon themes are validated but not
  rendered yet. Session restore covers editor documents only
  (not open panels, split state, or terminal scrollback).
- TypeScript is bundled for LSP. C/C++, Rust, and Python support require their
  language servers on `PATH`.

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
5. 🧩 **Data-only packs (M1 done):** theme, snippet, and icon packs now load
   in-process with enable/disable in Settings → EXTENSIONS. Remaining: icon
   rendering, keymap packs, a safe preview, and the `[packDir]` /
   `dwcode --check-pack` validator argument.
6. 🔐 **Extensions:** follow [`EXTENSIONS_BLUEPRINT.md`](EXTENSIONS_BLUEPRINT.md)
   — M0's `CommandRegistry` landed (menu-bar/status-bar models and the
   keybinding conflict compiler are still M3 work); M1 loads declarative packs;
   M2 adds the
   out-of-process host with the versioned JSON-RPC handshake, and M3/M4 add the
   narrow executable API behind the capability broker. Keep extensions isolated
   from the Qt app, Monaco page, filesystem, terminal, and network behind
   explicit RPC-granted permissions; distribution/registry work is M5. The
   candidate set to evaluate first is in
   [`FOSS_EXTENSION_REPOSITORY.md`](FOSS_EXTENSION_REPOSITORY.md).

Separate release verification remains worthwhile: test-install the newest Arch
package locally and verify launcher, window icon, bundled assets, and uninstall
behavior.
