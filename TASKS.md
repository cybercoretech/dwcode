# DWCode project checklist

## Prototype

- [x] Create the desktop app project in the Cybercore workspace and attach it to `cybercoretech/dwcode`.
- [x] Build a Qt Quick desktop shell around a locally bundled Monaco editor.
- [x] Add the first Cybercore identity mark and dark editor palette.
- [x] Prototype explorer, tabs, split view, command palette, output panels, and status bar.
- [x] Add a small optional Helix-inspired modal mode and keyboard shortcuts.
- [ ] Review the running UI and refine the layout from feedback.

## Editor foundation

- [x] Define the DWCode workspace and document model, then connect editor tabs and split views to shared documents.
- [x] Add safe local project open, file read, and atomic save with disk-conflict detection.
- [ ] Add file watching, a New File command, and recovery for unsaved work.
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
- [ ] Define the DWCode extension API, contribution points, permission model, and package format before loading third-party code.
- [ ] Connect the validated first-party pack to a DWCode theme/icon/snippet preview and selection UI.
- [ ] Decide the repository/package license before distributing any pack or logo asset.
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
- [ ] Package the app for Arch/Omarchy first, then add CI builds for other desktop platforms.
- [ ] Create the release icon pack, screenshots, install guide, and contribution documentation.
