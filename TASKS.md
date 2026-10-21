# DWCode project checklist

## Prototype

- [x] Create the desktop app project in the Cybercore workspace and attach it to `cybercoretech/dwcode`.
- [x] Build a Qt Quick desktop shell around a locally bundled Monaco editor.
- [x] Add the first Cybercore identity mark and dark editor palette.
- [x] Prototype explorer, tabs, split view, command palette, output panels, and status bar.
- [x] Add a small optional Helix-inspired modal mode and keyboard shortcuts.
- [ ] Review the running UI and refine the layout from feedback.

## Editor foundation

- [ ] Define the DWCode workspace and document model, then connect editor tabs and split views to shared documents.
- [ ] Add safe local project open, file read, atomic save, file watching, and recovery for unsaved work.
- [ ] Establish a versioned theme schema and ship cohesive dark, light, and high-contrast Cybercore themes.
- [x] Decide the extension-source policy: reviewed declarative packs may come from Open VSX; executable extensions require a DWCode host.
- [x] Draft manifest/API versioning and capability-permission model for future extensions.
- [x] Inspect the local VS Code extension and theme profile; record a prioritized DWCode support shortlist.
- [x] Create a first-party DWCode-style declarative pack prototype with original palettes, icons, snippets, and a local validator.
- [x] Build a ten-option DWCode logo selection gallery with SVG export for review.
- [x] Document the package format, visual adaptation rules, profile-based roadmap, and license gate.
- [ ] Define the DWCode extension API, contribution points, permission model, and package format before loading third-party code.
- [ ] Connect the validated first-party pack to a DWCode theme/icon/snippet preview and selection UI.
- [ ] Decide the repository/package license before distributing any pack or logo asset.
- [ ] Add Language Server Protocol support and let users discover/install compatible language servers.
- [ ] Add snippets, commands, keymaps, and theme packs as low-risk first extension types.
- [ ] Build an optional integration host with process isolation and explicit workspace permissions.
- [ ] Evaluate Open VSX declarative packages against DWCode schemas and package licenses; do not imply blanket VS Code extension compatibility.
- [ ] Prototype the curated GitHub Releases catalog and checksum-pinned index for DWCode-native packages.

## Developer workflow

- [ ] Implement Git status, diffs, search, problems/diagnostics, and an integrated terminal.
- [ ] Add task/run profiles and Debug Adapter Protocol support with clear trust prompts.
- [ ] Add tests for file operations, theme loading, command dispatch, editor bindings, and extension permissions.
- [ ] Package the app for Arch/Omarchy first, then add CI builds for other desktop platforms.
- [ ] Create the release icon pack, screenshots, install guide, and contribution documentation.
