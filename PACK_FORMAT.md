# 🧩 DWCode declarative packs

This page describes the first local pack prototype. DWCode does not install or execute VS Code extensions yet.

## 🛰️ Sources and compatibility

Open VSX may be used to discover candidate VSIX packages. A VSIX is only a transport package; it does not mean the included extension can run in DWCode. Each package needs a license and provenance review, integrity verification, schema validation, and contribution-by-contribution compatibility review.

The initial importer is limited to data: themes, icon maps and local SVGs, snippets, keymaps, language metadata, and supported syntax grammars. A package with executable `main` or `browser` entry points is not activated as a data pack. Prettier, ESLint, Git extensions, Python tooling, and similar executable favorites require native integrations or the future isolated extension host.

## 🎛️ DWCode visual adaptation

Third-party palettes are translated into the DWCode token schema. The window frame, navigation, panels, typography, spacing, focus rings, and interaction behavior stay under DWCode control. A source palette can influence syntax colors and editor-specific surfaces, but it cannot replace the Cybercore shell.

Icon packs are normalized to the DWCode icon canvas and subdued contrast. Do not recolor, alter, or redistribute third-party assets unless their license permits that use. If permission is unclear, build an original DWCode icon from broad design cues and retain an attribution and source note where appropriate. Pack settings and management UI use DWCode components.

## 🧰 Prototype pack contents

`packs/dwcode-core/manifest.json` declares the first local data pack:

- `DWCode Dark / Signal` — charcoal editor surfaces, muted cyan structure, and restrained rose, green, and amber syntax roles.
- `DWCode Frost / Signal` — a cool light palette with the same token names and quieter accent values.
- `DWCode Dark / Clear Signal` — higher contrast text and borders for readability.
- Seven original 24px SVG file/folder icons and associations for common web, Markdown, and data files.
- Small JavaScript, TypeScript, and Markdown snippets authored for this project.

These assets are custom DWCode work, not copied theme or icon files. The pack is MIT-licensed with DWCode and is not wired into a live theme picker or extension browser yet. Third-party assets remain subject to their own licenses and attribution requirements.

## 🔍 Validation

Run:

```sh
npm run validate:packs
```

The validator checks manifest version and id, rejects executable entry points and requested capabilities, checks that theme token sets match and use valid colors, confirms icon assets are local simple SVGs, and validates snippet shapes. It is a prototype validator; before accepting external packages, add a formal JSON Schema, archive extraction limits, checksum/signature handling, package-origin records, and adversarial tests.

## 🗺️ Implementation sequence

1. Keep the sample pack as the reference for DWCode's contribution schema.
2. Build a read-only package inspector that shows publisher, identifier, version, license, source, requested contributions, and checksum before installation.
3. Support only schema-validated data contributions; show preview and license/attribution details for imported assets.
4. Add undoable enable/disable/remove controls and a local exact-version lock record.
5. Implement native LSP and DAP integrations separately from VSIX activation.
6. Design and threat-model a versioned, out-of-process DWCode extension host before running extension code.
