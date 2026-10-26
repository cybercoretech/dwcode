# DWCode project checklist

## Prototype

- [x] Create the desktop app project in the Cybercore workspace and attach it to `cybercoretech/dwcode`.
- [x] Build a Qt Quick desktop shell around a locally bundled Monaco editor.
- [x] Add the first Cybercore identity mark and dark editor palette.
- [x] Prototype explorer, tabs, split view, command palette, output panels, and status bar.
- [x] Add a small optional Helix-inspired modal mode and keyboard shortcuts.
- [x] Review the running UI and refine the layout (real-display pass for Settings, palette, and right-side Explorer; fixed placeholder contrast and search-field affordance).

## Editor foundation

- [x] Define the DWCode workspace and document model, then connect editor tabs and split views to shared documents.
- [x] Add safe local project open, file read, and atomic save with disk-conflict detection.
- [x] Add file watching, a New File command, and recovery for unsaved work (session journal + restore, `6389275`).
- [ ] Establish a versioned theme schema and ship cohesive dark, light, and high-contrast Cybercore themes.
- [x] Decide the extension-source policy: reviewed declarative packs may come from Open VSX; executable extensions require a DWCode host.
- [x] Draft manifest/API versioning and capability-permission model for future extensions.
- [x] Inspect the local VS Code extension and theme profile; record a prioritized DWCode support shortlist.
- [x] Create a first-party DWCode-style declarative pack prototype with original palettes, icons, snippets, and a local validator.
- [x] Build a ten-option DWCode logo selection gallery with SVG export for review.
- [x] Select the Railshift mark and build its icon/avatar/favicon asset pack.
- [x] Replace the in-app mark with Railshift (top bar and window icon).
- [x] Publish the project site at dwcode.pages.dev with real screenshots and a browser demo.
- [x] Document the package format, visual adaptation rules, profile-based roadmap, and license gate.
- [x] Define the DWCode extension API, contribution points, permission model, and package format before loading third-party code (`EXTENSIONS.md` policy + `EXTENSIONS_BLUEPRINT.md` implementation design).
- [x] Implement blueprint phase M0: turn commands into the `CommandRegistry` the palette and `activate()` share (menu-bar and status-bar models, plus the keybinding conflict compiler, stay open for M3).
- [x] Implement blueprint phase M1: load validated declarative packs at runtime with an enable/disable switch in Settings → EXTENSIONS.
- [x] Research the FOSS shortlist on Open VSX and publish [`FOSS_EXTENSION_SHORTLIST.md`](FOSS_EXTENSION_SHORTLIST.md) (tiered candidates plus per-area coverage for needs, Rust, C++, Git, syntax highlighting, HTML/CSS, with licence and download verification).
- [ ] Connect the validated first-party pack to a DWCode theme/icon/snippet preview and selection UI.
- [x] License DWCode and first-party packs under MIT.
- [x] Add Language Server Protocol support: diagnostics, completion, hover, definition (TypeScript bundled; clangd, rust-analyzer, pyright from `PATH`).
- [ ] Let users discover and install compatible language servers.
- [ ] Add snippets, commands, keymaps, and theme packs as low-risk first extension types.
- [ ] Build an optional integration host with process isolation and explicit workspace permissions.
- [ ] Evaluate Open VSX declarative packages against DWCode schemas and package licenses; do not imply blanket VS Code extension compatibility.
- [ ] Prototype the curated GitHub Releases catalog and checksum-pinned index for DWCode-native packages.

## Developer workflow

- [x] Add a problems panel fed by language-server diagnostics and a workspace command runner.
- [ ] Replace the command runner with a real PTY terminal.
- [ ] Implement Git status, diffs, and workspace search.
- [ ] Add task/run profiles and Debug Adapter Protocol support with clear trust prompts.
- [x] Add backend tests (files, terminal, themes, LSP) and offscreen UI tests that drive the real window.
- [ ] Add tests for command dispatch, modal bindings, and extension permissions.
- [x] Package the app for Arch/Omarchy first and add CI-backed release artifacts.
- [x] Create the release icon pack, screenshots, and install guide.
