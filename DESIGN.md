# DWCode direction

DWCode is a standalone Cybercore desktop code editor. It owns its window chrome, workspace model, commands, settings, themes, integrations, and extension contract. Monaco is the first editing surface, not a promise of VS Code extension compatibility.

## Prototype architecture

- **Desktop shell:** C++ and Qt Quick/QML.
- **Editing surface:** Monaco Editor, bundled locally and hosted on a loopback-only HTTP server so editor workers can load offline.
- **Initial interaction model:** familiar explorer, tabs, split editor, command palette, output panels, and status bar, plus an optional Helix-inspired modal mode.
- **Identity:** DWCode wordmark, deep charcoal surfaces, muted borders, and restrained cyan/magenta accents. Keep the product recognizable without turning every control into a neon element.

## Integrations and extensions

Use Language Server Protocol for language intelligence and Debug Adapter Protocol for debugging. The prototype already speaks LSP over stdio for diagnostics, completion, hover, and definition. Keep language servers and debuggers as separately installed tools with explicit executable paths and workspace trust. Open VSX may supply reviewed, declarative themes, snippets, keymaps, and language metadata. It is not an executable extension host.

DWCode-native executable extensions come later, behind a versioned API, separate process, scoped permissions, explicit install/trust, and reviewable updates. Start the native catalog as a checksum-pinned GitHub Releases index. See [EXTENSIONS.md](EXTENSIONS.md) for the compatibility and trust policy.

## Brand system and sample contribution pack

- Keep application chrome in the Cybercore DWCode visual language: charcoal surfaces, muted separators, readable text, and sparing cyan/rose/green signals.
- Treat imported VS Code themes as source palettes to map into DWCode-owned tokens; do not let an imported theme replace the shell or typography.
- Normalize icons to DWCode's small canvas and restrained contrast. Preserve original licensing and attribution; create original assets if adaptation rights are not clear.
- The local `dwcode-core` pack demonstrates three palettes, seven file/folder icons, and starter snippets. It is not yet connected to runtime loading.
- The ten mark concepts in `brand/logo-selection.html` are a review gallery. Keep the existing app mark until the product owner selects and approves one concept.
- No project license is selected yet. The sample pack is therefore marked `UNLICENSED` and must not be distributed as a redistributable VSIX until licensing is resolved.
