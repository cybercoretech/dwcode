# 🌊 DWCode Editor

DWCode is Cybercore's keyboard-first desktop editor project. The prototype pairs a Qt Quick shell with a locally bundled Monaco editing surface, an optional Helix-inspired modal mode, and a restrained signal-and-wave visual identity.

## 🧭 Product direction

DWCode keeps a familiar application menu, project tree, tabs, splits, command palette, and lower panels, with optional modal navigation. The VS Codium-inspired layout preserves DWCode's own Cybercore palette and branding. Qt Quick owns the desktop shell; Monaco is the text editing surface. DWCode owns its workspace model, commands, settings, themes, integrations, and extension contract. Monaco does not make DWCode compatible with Visual Studio Code extensions by itself.

## ⚡ What works today

- 📂 **Workspace:** open any folder (`Ctrl+O` or `--workspace <folder>`), browse it in the explorer, and edit UTF-8 text files up to 8 MiB in tabs. Paths are confined to the workspace; symlinks that escape it are refused.
- 💾 **Saving:** `Ctrl+S` or **Save** writes atomically. If the file changed on disk since you opened it, the save is blocked so nothing is lost; use **Save As** for your copy. Unsaved changes prompt before closing or switching folders.
- 🖥️ **Terminal:** runs commands in the workspace with `bash`, streams output, accepts input for a running command, and can stop it. It is a command runner, not a full PTY, so full-screen programs such as `vim` or `htop` do not work yet.
- 🧠 **Language servers:** **Connect LSP** starts a server for the active file: TypeScript/JavaScript (bundled), C/C++ (`clangd`), Rust (`rust-analyzer`), Python (`pyright-langserver`). Diagnostics, completion, hover, and go-to-definition. TypeScript is covered by automated tests; the others depend on the server being on `PATH`.
- 🎨 **Themes:** 84 palettes, the 81 canonical Cybercore themes plus 3 DWCode ones, recolour the shell and editor together and are remembered between runs.
- 🪟 **Split view** shares the same document, so edits in one pane appear in the other.
- 🧭 **Application menu:** File, Edit, Selection, View, Run, Terminal, and Help expose the existing folder, save, editor, panel, language-server, terminal, and command-palette actions. The active tab is clearly marked with the current theme's primary accent.

Not yet: file watching, crash recovery, a New File command, a real PTY terminal, loading extension packs at runtime.

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

## 🧪 Tests

```sh
# Backend: files, conflicts, path boundaries, terminal, themes, TypeScript LSP
mkdir -p build/tests && cd build/tests && qmake6 ../../tests/backend.pro && make -j"$(nproc)" && ./backend-tests

# UI: drives the real window offscreen (dropdown, split view, save, terminal, LSP in the editor)
mkdir -p build/ui-tests && cd build/ui-tests && qmake6 ../../tests/ui.pro && make -j"$(nproc)" && ./ui-tests
```

The UI suite always runs offscreen so it never opens a window on your desktop; set `DWCODE_UI_PLATFORM` to watch it on a real display.

Validate the first data-only theme/icon/snippet pack with:

```sh
npm run validate:packs
```

## 🌐 Website

The project site, **[dwcode.pages.dev](https://dwcode.pages.dev)**, lives in [`site/`](site/): features, real window captures, all 84 themes, the Railshift downloads, and a live panel running the app's Monaco bundle in the browser (sample files only; saving, the terminal and language servers need the desktop app).

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

The first pack at [`packs/dwcode-core`](packs/dwcode-core) is an original DWCode contribution set with Dark, Frost, and Clear Signal palettes, muted file icons, and JavaScript/TypeScript/Markdown starter snippets. It is a data-format prototype, not yet loaded into the running app. The validator confirms that it contains no executable entry point and that its local resources and theme tokens are valid.

Open VSX is a discovery and package source, not a compatibility guarantee. DWCode will begin with reviewed declarative data such as theme, icon, snippet, keymap, and grammar contributions. Executable VSIX extensions wait for a DWCode-native isolated host and a versioned permission broker. Read [`EXTENSIONS.md`](EXTENSIONS.md), [`USER_EXTENSION_PROFILE.md`](USER_EXTENSION_PROFILE.md), and [`PACK_FORMAT.md`](PACK_FORMAT.md) for the selection, design, and compatibility decisions.

Every pack must look and behave like DWCode: the Cybercore shell remains consistent, palettes map into DWCode tokens, icons use the app's restrained sizing and contrast, and upstream license and attribution terms are retained. The first-party starter pack is MIT-licensed with the project; third-party packs still require independent license and attribution review before distribution.

## 📚 Project notes

- [Design decisions](DESIGN.md)
- [Extension API and package policy](EXTENSIONS.md)
- [Declarative pack format](PACK_FORMAT.md)
- [VS Code profile and integration shortlist](USER_EXTENSION_PROFILE.md)
- [Project checklist](TASKS.md)
