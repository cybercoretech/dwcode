# DWCode extensions: implementation blueprint

**Status:** design for implementation · **Companion to:** [EXTENSIONS.md](EXTENSIONS.md) (policy),
[PACK_FORMAT.md](PACK_FORMAT.md) (declarative pack schema),
[USER_EXTENSION_PROFILE.md](USER_EXTENSION_PROFILE.md) (target-workload inventory)

EXTENSIONS.md decides *what* DWCode will and will not accept. This document says *how* to build it
against the code that exists today, seam by seam, with milestones that each land green.

Three rules from the policy carry into every phase:

1. Declarative data loads in-process; executable code never does.
2. No extension sees Qt/QML objects, raw paths, or an open channel — only capability-scoped RPC.
3. The Cybercore shell, theme tokens, and interaction patterns stay DWCode-owned. Contributions
   render through DWCode components and the existing `theme` tokens; no hard-coded colors.

---

## 1. Ground truth: the seams that already exist

| Seam | Where it lives today | What extensions need from it |
| --- | --- | --- |
| C++ ↔ Monaco bridge | `src/qml/Main.qml:106-107` — `WebChannel { registerObject("native", workspaceBackend) }`; consumed at `src/main.js:283-302` (`new QWebChannel(...)`) | The `native` object is the only sanctioned crossing. Extension RPC must **not** ride on it; a second, separately authenticated channel is introduced in §4. |
| Asset transport | `src/localwebserver.cpp:52-89` — loopback HTTP, **GET-only** (405 otherwise), canonical-path containment, `nosniff` | Stays GET-only. Extension code is never served over this port; pack *assets* (icons, snippets, themes) are, after validation. |
| Command surface | `src/qml/Main.qml:26-40` (`property var commands`), `activate(action)` at `Main.qml:46-64` | Must become a registry with string IDs (`ext.<id>.<cmd>`) instead of a closed `if/else` on action names. |
| Command palette | `Main.qml:278-286` (`paletteOverlay`, `commandInput`, `ListView { model: win.commands }`) | Reads the registry; extension commands appear automatically, with `contributes.commands` titles and keybinding labels. |
| Keybindings | `Main.qml:114-121` (`Shortcut { sequence: ... }`) | Declarative `contributes.keybindings` compile into `Shortcut` items from the registry, with conflict detection. |
| Menus | `Main.qml:131-169` (static `MenuBar` / `MenuItem` tree) | Needs an `extensions` slot per top-level menu, fed by `contributes.menus`. |
| Status bar | `Main.qml:271-276` (single `RowLayout` with message + brand label) | Becomes a model-driven `Repeater` so `ui.statusItem` contributions land in DWCode-controlled slots. |
| Settings funnel | `WorkspaceBackend::setPreference` / `preferencesChanged` → `src/main.js:111` (`window.dwcodeApplySettings`) and `main.js:292,300` | Extension-scoped settings use the same store under `settings/ext/<id>/<key>` and the same live-apply path. |
| Validation | `tools/validate-packs.mjs` (schema checks, escapes-the-pack-dir guard, executable-entry rejection) | Generalized to accept any pack directory + manifest, so install-time validation and CI share one code path. |
| Versioned on-disk format precedent | Session journal (`src/workspacebackend.cpp`, `session.json` `version: 1`, read-once in the constructor, caps on tabs/bytes) | The extension install/lock record and grant store follow the same pattern: integer version, conservative caps, fail-closed on unknown version. |

Existing test scaffolding that each phase must extend rather than replace:
`tests/backend_test.cpp` (logic, no GUI), `tests/ui_test.cpp` + `tests/ui_harness.h` (real
`Main.qml`, objectName-based lookups, `launch(app, keepSession)`), `tests/shots.cpp` (real-display
captures for layout sign-off).

---

## 2. Target architecture

```
┌─ DWCode (Qt process) ──────────────────────────────────────────────┐
│  WorkspaceBackend  ── session, files, LSP, preferences (existing)  │
│  ExtensionBroker   ── NEW: spawn/supervise hosts, check every call │
│  ContributionRegistry (QML) ── commands, menus, status, keybindings│
└───────────────┬────────────────────────────────────────────────────┘
                │  JSON-RPC 2.0, newline-delimited, over a QProcess
                │  stdio pipe (no listening socket, no web channel)
┌───────────────▼────────────────────────────────────────────────────┐
│  extension host process (one per extension, Linux namespaces/seccomp│
│  where available) — runs extension JS in a minimal runtime, replies │
│  to handshake, emits contribution registrations                     │
└────────────────────────────────────────────────────────────────────┘
```

Why stdio instead of the existing HTTP server: `localwebserver.cpp` is deliberately GET-only and
serves only bundled assets. Opening a bidirectional channel there would widen the Monaco page's
attack surface for no benefit. `QProcess` stdio gives an authenticated, per-extension, inheritable
channel with no port at all.

---

## 3. Phase M0 — contribution seams (no extension code yet)

Refactor the closed surfaces into registries, with zero visible behavior change. This is the
prerequisite for every later phase and is independently shippable.

**Changes**

- `src/qml/Main.qml`
  - Replace `property var commands` with a `List`-backed `CommandRegistry` (a small `QtObject` in
    a new `src/qml/CommandRegistry.qml`): `register(id, {title, shortcut, action|handler, source})`,
    `unregister(id)`, `run(id)`. Built-in commands self-register in `Component.onCompleted` with
    ids like `dwcode.file.save`.
  - `activate(action)` keeps its name for built-ins; unknown ids route to the registry and invoke
    the handler (built-in handlers wrap today's `if/else` bodies verbatim).
  - Palette `ListView` and the `Shortcut` rows read the registry, so a registered command is
    immediately palette-searchable and, if it declares a sequence, bound.
  - Menu bar: add `Menu` entries for `contributes.menus` slots (`file`, `edit`, `selection`, `view`,
    `run`, `terminal`, `settings`, `help`) driven by a model.
  - Status bar: convert the two hard-coded `Label`s into `Repeater { model: statusModel }` where
    built-ins are the first two entries; DWCode controls order, padding, font size, and the
    `theme.*` color bindings — extension items get text + click only, per the capability table.
- Keybinding compiler: parse `"ctrl+shift+x"` style sequences, detect conflicts with built-in
  sequences, and surface conflicts in Settings (never silently override a built-in).

**Tests**

- `backend_test`: none required (pure QML), but add a `contributions` case once M1 lands.
- `ui_test`: new `commandRegistryDrivesPaletteAndMenus` — register a synthetic command, assert it
  appears in the palette list, run it via `win.activate("test.cmd")`, assert the side effect, then
  unregister and assert it disappears. Also assert status-bar model length changes.
- `shots.cpp` `layout()`: unchanged captures must be pixel-stable (registry refactor must not
  move the status bar or palette).

---

## 4. Phase M1 — load declarative packs (first user-visible extensions)

Bring `packs/dwcode-core` from "validates" to "loads", still data-only, still in-process.

**Changes**

- New `src/packloader.{h,cpp}`:
  - Discovery: `<data dir>/packs/`, plus user install dir `<qsettings path>/<org>/<app>.packs/`
    (same isolation trick the session journal uses, so tests inherit `QSettings::setPath`).
  - Parse `manifest.json` with the *same* rules as `tools/validate-packs.mjs`: `manifestVersion`
    must be 1, `kind: "declarative"`, reject `main`/`browser`, reject non-empty `capabilities`,
    require `license`. C++ re-validation is mandatory — the Node tool runs in CI, the app must not
    trust files that merely passed CI.
  - Themes are mapped into the existing 84-theme catalog schema (`src/data/themes.json` shape,
    `schemaVersion: 1`, token groups `surfaces/text/accents/syntax`) so `setTheme`, Monaco, and
    `Theme.qml` need no changes. Application chrome tokens are overwritten from DWCode defaults,
    never imported verbatim (EXTENSIONS.md §Stage 2).
  - Snippets/languages/keymaps are handed to Monaco through the existing settings funnel: extend
    `window.dwcodeApplySettings` to also accept `packs/snippets` / `packs/languages` and call
    `monaco.languages.register` / `monaco.languages.setMonarchTokensProvider` /
    `monaco.editor.registerSnippets`-equivalents in `src/main.js`.
- Settings dialog: new **EXTENSIONS** section — list packs, publisher, version, license, source
  hash, enable/disable, reset. Enable state in `settings/packs/<id>/enabled`.
- Generalize `tools/validate-packs.mjs` to `node tools/validate-packs.mjs [packDir]` so any
  installed pack can be checked by CI and by an optional `dwcode --check-pack <dir>` CLI flag.

**Tests**

- `backend_test`: `packLoader` — good pack loads, `main` field rejected, unknown
  `manifestVersion` rejected, theme merged into `themes()` with chrome tokens restored to DWCode
  defaults, enable/disable reflected in `setTheme` options, pack dir traversal (`../`) rejected.
- `ui_test`: EXTENSIONS section renders, toggle disables a theme in the dropdown.
- `npm run validate:packs` keeps guarding `packs/dwcode-core` in CI.

---

## 5. Phase M2 — extension host skeleton + handshake

No capabilities yet. Prove process lifecycle, versioning, and fail-closed behavior.

**Changes**

- New `src/extensionbroker.{h,cpp}` (C++, `QProcess`-driven):
  - One host process per executable extension, spawned with `--extension-id <id> --version <v>`;
    stdout/stdin are newline-delimited JSON-RPC 2.0. stderr is captured into the Output panel.
  - **Handshake (per EXTENSIONS.md):**
    1. Host → DWCode: `initialize { manifestVersion, dwcodeApi, engines.dwcode, capabilities[] }`.
    2. DWCode → host: `initialize.result { dwcodeApi, grantedCapabilities[], workspaceTrust,
       locale, themeTokens }` — only after manifest validation, hash check, and per-capability
       grants read from the grant store.
    3. Host → DWCode: `initialized` notification, then `workspace/activate` events.
    4. Anything sent before `initialized`, an unknown method, an ungranted capability, or a
       version outside the declared range → error reply + host kill. **Fail closed.**
  - Three independent version axes stay independent: `manifestVersion` (package schema int),
    `engines.dwcode` (app SemVer range), `dwcodeApi` (API major.minor). Stability levels
    `proposed | preview | stable`; `preview` methods require `{"apiLevel":"preview"}` opt-in in
    the handshake.
  - Supervision: heartbeat ping every 10 s, 3 missed pings kills the host, crash-loop backoff,
    `disableOnCrash` after 5 restarts in 60 s, host stopped on workspace close / app exit /
    extension disable (mirrors `terminateGroup` for terminal/LSP in `workspacebackend.cpp`).
- Grant store: `QSettings` key `extensions/grants/<id>` → `{hash, capabilities[], grantedAt}`.
  Install or update whose package hash differs from the stored hash re-prompts (EXTENSIONS.md
  "updates that add capabilities require consent again").

**Tests**

- `backend_test`: fake host fixtures under `tests/fixtures/host/` (a tiny Node or shell script
  speaking the protocol):
  - happy handshake → `status == activated`;
  - wrong `manifestVersion` → rejected, process killed;
  - sending `commands/register` before `initialized` → rejected;
  - unknown method → error reply, host killed;
  - heartbeat timeout → supervisor kills and reports;
  - grant removed → next request rejected and host stopped.
- No UI changes; `ui_test` must stay 13/13.

---

## 6. Phase M3 — first executable API (narrow, stable-when-ready)

Implement exactly the v1 scope fixed by EXTENSIONS.md: register commands and keybindings, read
(extension-scoped) settings, show notifications, contribute a status item, register cancellable
language providers. Workspace file read/edit, terminal, process, network, and webviews stay
excluded — the broker rejects those method names outright so the rejection is testable.

**Broker surface (JSON-RPC methods)**

| Method | Capability | Broker checks |
| --- | --- | --- |
| `commands/register` | `commands.register` | id must be `<extid>.<name>`; max 100; palette/menus updated via `ContributionRegistry` in QML |
| `commands/invoke` (DWCode → host) | — | per-invocation timeout; result rendered by DWCode, never HTML |
| `keybindings/set` | `commands.register` | conflict check against built-ins; sequence normalized |
| `settings/read` / `settings/write` | `settings.read` | keys confined to `settings/ext/<id>/` |
| `notifications/show` | — | text-only, DWCode-styled `Message`-style surface, rate-limited |
| `statusItem/set` | `ui.statusItem` | text + optional icon (DWCode normalizes size); click routes to a registered command |
| `languages/*` | (M4) | cancellation, per-request deadline, output capped |

**Threading:** broker runs on the GUI thread but posts to a worker `QSocketNotifier`/`QIODevice`
read loop; no extension call may block the event loop — every reply has a deadline and pending
calls are tracked by id for `$/cancelRequest`.

**Tests**

- `backend_test`: one case per row above (grant present → applied; grant absent → `error -32002`
  style code; oversized message (> 1 MiB) dropped; rate-limit on notifications).
- `ui_test`: `extensionContributionsRender` — inject a synthetic registered command + status item
  through the registry (no host needed), assert palette entry, status-bar slot, and that the
  rendered item uses theme tokens (check the item's `color` equals `theme.cyan`, not a literal).
- `shots.cpp`: new `layout()` capture of a status-item + extension-command palette for visual
  sign-off on the real display.

---

## 7. Phase M4 — providers, limits, and threat-model review

- Language providers (completion/hover/diagnostics) over the same channel with cancellation and a
  hard per-request deadline; results validated structurally before touching Monaco markers
  (reuse the marker path at `src/main.js:294-298`).
- Resource limits: wall-clock and CPU-time kill switches, message-size caps, in-flight request
  caps, and a per-host memory ceiling where the platform allows (`setrlimit`/cgroups on Linux).
  If a platform cannot deliver the promised isolation, executable extensions stay disabled there
  while declarative packs continue to work (EXTENSIONS.md capability section).
- Deliver the threat model + adversarial tests called out at the end of EXTENSIONS.md: path
  traversal through workspace handles, RPC injection via control characters, resource exhaustion,
  workspace-trust downgrade, and supply-chain (lock-record hash mismatch).

**Tests:** the adversarial list above becomes a dedicated `backend_test` function each; CI runs
the suite with executable extensions forced on.

---

## 8. Phase M5 — distribution

- Curated catalog: signed/checksum-pinned JSON index on GitHub Releases; exact-version installs
  with origin + sha256 written to a lock record (mirrors the session journal's versioned,
  fail-closed reads). Updates are opt-in and show the capability diff before re-consent.
- No URL-based installs by default; local VSIX/file import, if added, runs the identical
  manifest + license + integrity + compatibility checks.
- Compatibility report per package: `declarative-only` / `needs language server` / `requires
  DWCode host`, rendered in the EXTENSIONS settings section.

---

## 9. File-by-file change list

| Phase | New | Modified |
| --- | --- | --- |
| M0 | `src/qml/CommandRegistry.qml` | `src/qml/Main.qml`, `tests/ui_test.cpp` |
| M1 | `src/packloader.{h,cpp}` | `dwcode.pro`, `src/main.js`, `src/qml/SettingsDialog.qml`, `src/resources.qrc`, `tools/validate-packs.mjs`, `tests/backend_test.cpp` |
| M2 | `src/extensionbroker.{h,cpp}`, `tests/fixtures/host/*` | `src/main.cpp`, `tests/backend_test.cpp` |
| M3 | `src/qml/ExtensionStatusDelegate.qml` (or equivalent) | `src/extensionbroker.cpp`, `src/qml/Main.qml`, `tests/{backend,ui}_test.cpp`, `tests/shots.cpp` |
| M4 | `tests/threatmodel.md` | `src/extensionbroker.cpp`, `tests/backend_test.cpp` |
| M5 | `tools/fetch-catalog.mjs` | `src/packloader.cpp`, docs |

Every phase keeps the three suites green: `./backend-tests` (currently 9/9),
`./ui-tests` (currently 13/13), `npm run build && npm run validate:packs`.

---

## 10. Open questions (decide before M2)

1. **Host runtime.** Node.js on PATH vs. a bundled runtime vs. QuickJS-in-C++. A bundled runtime
   removes a system dependency but adds ~40 MB and release plumbing; QuickJS keeps the trust
   surface small but excludes packages that assume Node APIs. Recommendation: abstract the host
   behind the stdio protocol so the runtime choice is swappable, and start with system Node
   behind a capability check (`process.exec` for the host itself is DWCode's own spawn, not an
   extension grant).
2. **One host per extension vs. one shared host.** Per-extension isolation is the safer default;
   a shared host saves memory but lets extensions share a fate. Recommendation: per-extension,
   with a documented memory budget before M3.
3. **Menu scope for extensions in v1.** Commands + palette + keybindings first; menu-bar slots can
   wait for M3 if M0's menu model is not needed earlier.
