# 🌊 DWCode Editor

DWCode is Cybercore's keyboard-first desktop editor project. The prototype pairs a Qt Quick shell with a locally bundled Monaco editing surface, an optional Helix-inspired modal mode, and a restrained signal-and-wave visual identity.

## 🧭 Product direction

DWCode keeps a familiar application menu, project tree, tabs, splits, command palette, and lower panels, with optional modal navigation. The VS Codium-inspired layout preserves DWCode's own Cybercore palette and branding. Qt Quick owns the desktop shell; Monaco is the text editing surface. DWCode owns its workspace model, commands, settings, themes, integrations, and extension contract. Monaco does not make DWCode compatible with Visual Studio Code extensions by itself.

## ⚡ What works today

- 📂 **Workspace:** open any folder (`Ctrl+O` or `--workspace <folder>`), browse it in the explorer, create unsaved files (`Ctrl+N`), and edit UTF-8 text files up to 8 MiB in tabs. Paths are confined to the workspace; symlinks that escape it are refused.
- 🗂️ **Explorer controls:** the Explorer header provides **Open Folder** and refresh controls, and the native window uses the Railshift app icon.
- ♻️ **Session restore:** the open tabs, their order, the active file, and unsaved buffers are journaled as you type and come back after a restart or crash (**Keep Changes** on close keeps them; a new `files/restoreSession` setting turns it off). If a file changed on disk while DWCode was closed, saving the restored buffer is refused and reported as an external change instead of overwriting it.
- 💾 **Saving:** `Ctrl+S` or **Save** writes atomically. Workspace changes refresh the Explorer, and an open file changed or deleted outside DWCode prompts you to reload or keep your editor copy. Saving an externally changed file remains blocked so nothing is lost; use **Save As** for your copy. Unsaved changes prompt before closing or switching folders, and closing offers **Keep Changes** so the work reappears next launch.
- 🖥️ **Terminal:** runs commands in the workspace with `bash`, streams output, accepts input for a running command, and can stop it. It is a command runner, not a full PTY, so full-screen programs such as `vim` or `htop` do not work yet.
- 🧠 **Language servers:** **Connect LSP** starts a server for the active file: TypeScript/JavaScript (bundled), C/C++ (`clangd`), Rust (`rust-analyzer`), Python (`pyright-langserver`). Diagnostics, completion, hover, and go-to-definition. TypeScript is covered by automated tests; the others depend on the server being on `PATH`.
- 🎨 **Themes:** 84 palettes, the 81 canonical Cybercore themes plus 3 DWCode ones, recolour the shell and editor together and are remembered between runs.
- ⚙️ **Settings:** **Settings → Preferences…** (`Ctrl+,`) opens searchable,
  persistent controls for appearance, Monaco, save behavior, workspace files,
  the command runner, and modal editing.
- 🪟 **Split view** shares the same document, so edits in one pane appear in the other.
- 🧭 **Application menu:** File, Edit, Selection, View, Run, Terminal, Settings,
  and Help expose the existing folder, save, editor, panel, language-server,
  terminal, command-palette, and preference actions. The active tab is clearly
  marked with the current theme's primary accent.

Not yet: a real PTY terminal, loading extension packs at runtime, and
executable extensions (see the extension blueprint).

## 🛠️ Build and run

Requirements: Qt 6 with Qt Quick Controls and Qt WebEngine Quick, qmake6, a C++17 compiler, Node.js, and npm.

```sh
npm ci
./bin/build
./build/dwcode --workspace ~/some/project
```

The Monaco UI is built into `web/dist` and served from a loopback-only HTTP listener so its editor workers run without loading code from a third-party site. Native capabilities are exposed to the editor only through Qt WebChannel, never on that HTTP listener.

## 🐧 Arch / Omarchy alpha

DWCode has a local-first Arch package for its alpha release. It installs the
binary, Railshift launcher icon, desktop entry, and bundled Monaco assets, so
the installed app does not depend on a source checkout.

```sh
./packaging/arch/build-package.sh
sudo pacman -U dist/dwcode-0.1.0-1-x86_64.pkg.tar.zst
```

Launch **DWCode** from the application launcher or run `dwcode`. To remove it
cleanly, run `sudo pacman -Rns dwcode`. See
[the Arch installation guide](docs/install-arch.md) for prerequisites, updates,
and the current alpha limitations.

### Arch User Repository

The same two packages are scaffolded for the AUR under
[`packaging/aur/`](packaging/aur/) — `dwcode` (source build) and `dwcode-bin`
(prebuilt, `provides`/`conflicts` on `dwcode`), each with a generated
`.SRCINFO`. Releases are cut by one command:

```sh
tools/aur-release.sh --dry-run   # bump pkgver, recompute both sha256sums,
                                  # regenerate .SRCINFO, show the AUR pushes
tools/aur-release.sh             # …and actually push to
                                  # ssh://aur@aur.archlinux.org/dwcode.git
```

The script refuses to publish while any `sha256sums` is still `SKIP`, so an
unverifiable package cannot reach the AUR by accident. Full flow, SSH setup,
and the `-bin` asset recipe are in
[`packaging/aur/README.md`](packaging/aur/README.md).

## 🧪 Tests

```sh
# Backend: files, conflicts, path boundaries, terminal, themes, pack loading, TypeScript LSP, session restore (10 checks)
mkdir -p build/tests && cd build/tests && qmake6 ../../tests/backend.pro && make -j"$(nproc)" && ./backend-tests

# UI: drives the real window offscreen (dropdown, split view, save, terminal, LSP, extensions section; 14 checks)
mkdir -p build/ui-tests && cd build/ui-tests && qmake6 ../../tests/ui.pro && make -j"$(nproc)" && ./ui-tests
```

The UI suite always runs offscreen so it never opens a window on your desktop; set `DWCODE_UI_PLATFORM` to watch it on a real display.

Validate the first data-only theme/icon/snippet pack, the catalog licence gate, and the grammar converter with:

```sh
npm run validate:packs   # pack manifest, theme token sets, local SVG icons, snippet shapes
npm run catalog:scan     # licence gate: every extension id in FOSS_EXTENSION_REPOSITORY.md
                         # re-checked against the live Open VSX registry; exits 1 while a
                         # licence is still pending (refusals/absent ids are recorded, not failures)
npm run catalog:build    # gate + curated content -> site/data/catalog.json (network; 172 cards)
npm run catalog:render   # paint that data into site/repository.html between the CATALOG markers
npm run validate:catalog # offline: data <-> HTML <-> repository doc must agree, refused ids absent
npm run grammar:test     # tools/tm2monarch.mjs --self-test: 16 TextMate → Monarch conversion
                         # cases, each emitted definition also compiled by Monaco's monarchCompile.js
```

## 🌐 Website

The project site, **[dwcode.pages.dev](https://dwcode.pages.dev)**, lives in [`site/`](site/): features, real window captures, all 84 themes, the Railshift downloads, and a live panel running the app's Monaco bundle in the browser (sample files only; saving, the terminal and language servers need the desktop app). [`site/repository.html`](site/repository.html) is the second page — *Open DWCode Extension Repository*. It opens with the **catalog itself**: 172 licence-checked cards in 17 families with a search field, a category rail and four sort orders, each card carrying name, publisher, version, licence chip (linking the publisher's repository), downloads, a copy-id button and a disabled `install · M5` button. Below it sit the policy sections — how the checksum-pinned index works, the licence-gate table, the excluded packages with reasons, how to submit a pack, and the attribution/credits section. The grid is static HTML painted by `npm run catalog:render` from `site/data/catalog.json`, so the page still reads correctly with JavaScript off; `site/catalog.js` only adds the filtering on top. It shares the same `site.js`/`styles.css`, so both pages must stay error-free together.

```sh
./tools/build-site.sh --shots   # recapture screenshots offscreen, build the demo, assemble site/
./tools/deploy-site.sh          # build and upload to Cloudflare Pages (needs wrangler login)
```

Screenshots come from `tests/shots.cpp`, which drives the real window through the same harness as the UI tests and saves 2x captures to `build/site-shots/`.

## 🛰️ Brand: Railshift

**Railshift** is the approved DWCode mark. The asset pack in [`brand/railshift/`](brand/railshift/) has transparent PNGs and dark avatars from 64 to 1024 px, a multi-resolution ICO, favicons, Apple/Android icons, a social card, and simplified true-vector SVGs (`railshift-vector.svg`, `railshift-mono.svg`, `railshift-favicon.svg`). `railshift-artwork.svg` wraps the detailed raster and is not a true vector. Rebuild the pack and its zip with `python3 tools/build-brand-assets.py`.

The exploration that led there is kept for reference: [`brand/logo-selection.html`](brand/logo-selection.html) (the five fusion concepts, Railshift is option 04), [`brand/fusion-concepts/`](brand/fusion-concepts/), [`brand/neon-concepts/`](brand/neon-concepts/), and the earlier [concept board](brand/dwcode-logo-options.png) with its GIMP source.

## ⌨️ Prototype controls

| Shortcut | Action |
| --- | --- |
| `Ctrl+O` | Open a folder |
| `Ctrl+S` / `Ctrl+Shift+S` | Save / Save As |
| `Ctrl+P` | Open the command palette / quick switcher |
| `Ctrl+Shift+P` | Open the command palette |
| `Ctrl+B` | Toggle the explorer |
| `Ctrl+J` | Toggle the bottom panel |
| `Ctrl+\\` | Split the editor surface |
| `Ctrl+Alt+M` | Toggle Helix-inspired modal editing |

In modal mode, `h`, `j`, `k`, and `l` move the cursor, `x` deletes the character to the right, and `i` returns to insert mode.

## Direction

DWCode will own its workspace model, command system, settings, theme schema, and integration APIs. Monaco is only the text-editing surface; it does not provide VS Code extension compatibility. The initial UI uses a Cybercore palette with restrained cyan and magenta signals, and keeps editor chrome separate from the editing engine.

## 🧩 Extension plan

The first pack at [`packs/dwcode-core`](packs/dwcode-core) is an original DWCode contribution set with Dark, Frost, and Clear Signal palettes, muted file icons, and JavaScript/TypeScript/Markdown starter snippets. It loads in-process as data: the backend re-validates it with the same rules as the Node validator, its three themes merge into the 84-theme catalog, its snippets reach Monaco, and Settings → EXTENSIONS can switch the whole pack off. The validator confirms that it contains no executable entry point and that its local resources and theme tokens are valid.

Open VSX is a discovery and package source, not a compatibility guarantee. DWCode will begin with reviewed declarative data such as theme, icon, snippet, keymap, and grammar contributions. Executable VSIX extensions wait for a DWCode-native isolated host and a versioned permission broker. Read [`EXTENSIONS.md`](EXTENSIONS.md) for the policy, [`EXTENSIONS_BLUEPRINT.md`](EXTENSIONS_BLUEPRINT.md) for the phased implementation design (contribution registries → declarative loader → out-of-process host and capability broker), [`USER_EXTENSION_PROFILE.md`](USER_EXTENSION_PROFILE.md) for the selection decisions, [`FOSS_EXTENSION_REPOSITORY.md`](FOSS_EXTENSION_REPOSITORY.md) for the verified FOSS candidates on Open VSX, and [`PACK_FORMAT.md`](PACK_FORMAT.md) for the pack schema.

Every pack must look and behave like DWCode: the Cybercore shell remains consistent, palettes map into DWCode tokens, icons use the app's restrained sizing and contrast, and upstream license and attribution terms are retained. The first-party starter pack is MIT-licensed with the project; third-party packs still require independent license and attribution review before distribution.

## 📚 Project notes

- [Design decisions](DESIGN.md)
- [Extension API and package policy](EXTENSIONS.md)
- [Extensions implementation blueprint](EXTENSIONS_BLUEPRINT.md)
- [Declarative pack format](PACK_FORMAT.md)
- [VS Code profile and integration shortlist](USER_EXTENSION_PROFILE.md)
- [FOSS extension repository — licence-verified catalog list on Open VSX](FOSS_EXTENSION_REPOSITORY.md)
- [Phased report — Install (M5), the DWCode API recommendation, and the agentforge/ferraxis/diagprint assessments](docs/report-m5-api-roadmap.md)
- [Project checklist](TASKS.md)
