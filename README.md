# 🌊 DWCode Editor

DWCode is Cybercore's keyboard-first desktop editor project. The prototype pairs a Qt Quick shell with a locally bundled Monaco editing surface, an optional Helix-inspired modal mode, and a restrained signal-and-wave visual identity.

## 🧭 Product direction

DWCode keeps a familiar project tree, tabs, splits, command palette, and lower panels, with optional modal navigation. Qt Quick owns the desktop shell; Monaco is the text editing surface. DWCode owns its workspace model, commands, settings, themes, integrations, and extension contract. Monaco does not make DWCode compatible with Visual Studio Code extensions by itself.

The current prototype uses a sample project. It does not yet open arbitrary folders, save files, start language servers, run/debug project code, or load third-party VSIX files.

## Build and run

Requirements: Qt 6 with Qt Quick Controls and Qt WebEngine Quick, qmake6, a C++17 compiler, Node.js, and npm.

```sh
npm install
./bin/build
./build/dwcode
```

The Monaco UI is built into `web/dist` and served from a loopback-only HTTP listener so its editor workers function without loading code from a third-party site. The prototype has a sample workspace and interaction scaffolding; project file operations, language-server integration, and runtime execution are not connected yet.

Validate the first data-only theme/icon/snippet pack with:

```sh
npm run validate:packs
```

Open [`brand/logo-selection.html`](brand/logo-selection.html) in a browser to compare the ten original DWCode mark directions. Select symbol-only or full wordmark-lockup SVG, then copy or download it. The selected mark has not yet replaced the app's current logo.

## Prototype controls

| Shortcut | Action |
| --- | --- |
| `Ctrl+P` | Open the command palette / quick switcher |
| `Ctrl+Shift+P` | Open the command palette |
| `Ctrl+B` | Toggle the explorer |
| `Ctrl+J` | Toggle the bottom panel |
| `Ctrl+\\` | Split the editor surface |
| `Ctrl+Shift+M` | Toggle Helix-inspired modal editing |

In modal mode, `h`, `j`, `k`, and `l` move the cursor, `x` deletes the character to the right, and `i` returns to insert mode.

## Direction

DWCode will own its workspace model, command system, settings, theme schema, and integration APIs. Monaco is only the text-editing surface; it does not provide VS Code extension compatibility. The initial UI uses a Cybercore palette with restrained cyan and magenta signals, and keeps editor chrome separate from the editing engine.

## 🧩 Extension plan

The first pack at [`packs/dwcode-core`](packs/dwcode-core) is an original DWCode contribution set with Dark, Frost, and Clear Signal palettes, muted file icons, and JavaScript/TypeScript/Markdown starter snippets. It is a data-format prototype, not yet loaded into the running app. The validator confirms that it contains no executable entry point and that its local resources and theme tokens are valid.

Open VSX is a discovery and package source, not a compatibility guarantee. DWCode will begin with reviewed declarative data such as theme, icon, snippet, keymap, and grammar contributions. Executable VSIX extensions wait for a DWCode-native isolated host and a versioned permission broker. Read [`EXTENSIONS.md`](EXTENSIONS.md), [`USER_EXTENSION_PROFILE.md`](USER_EXTENSION_PROFILE.md), and [`PACK_FORMAT.md`](PACK_FORMAT.md) for the selection, design, and compatibility decisions.

Every pack must look and behave like DWCode: the Cybercore shell remains consistent, palettes map into DWCode tokens, icons use the app's restrained sizing and contrast, and upstream license and attribution terms are retained. The starter pack is marked `UNLICENSED` until the repository owner selects a project license; it must not be published as a redistributable package before that choice.

## 📚 Project notes

- [Design decisions](DESIGN.md)
- [Extension API and package policy](EXTENSIONS.md)
- [Declarative pack format](PACK_FORMAT.md)
- [VS Code profile and integration shortlist](USER_EXTENSION_PROFILE.md)
- [Project checklist](TASKS.md)
