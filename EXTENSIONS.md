# DWCode extension and integration policy

## Decision

DWCode will have its own extension contract. Open VSX is an optional discovery and download source for a deliberately limited set of declarative packages; it is not a promise to run arbitrary VS Code extensions. A package is not compatible merely because it is a VSIX or declares an `engines.vscode` range.

The Visual Studio Marketplace is not a source for DWCode. Microsoft documents it as intended for Visual Studio-family products, and marketplace packages can have separate licenses and use conditions.

The first DWCode-authored data-only reference pack is `packs/dwcode-core`. It demonstrates the visual and contribution rules, is MIT-licensed with DWCode, and loads in-process as data: the backend re-validates it, its themes merge into the catalog, its snippets reach the editor, and Settings → EXTENSIONS can switch it off. See [PACK_FORMAT.md](PACK_FORMAT.md) for its contents and validation limits, and [FOSS_EXTENSION_REPOSITORY.md](FOSS_EXTENSION_REPOSITORY.md) for which third-party packages are candidates for the same treatment.

## Release stages

### Stage 1: built-in integrations

- Implement Language Server Protocol (LSP) clients for completion, diagnostics, hover, navigation, and formatting.
- Implement Debug Adapter Protocol (DAP) after the editor and task model are stable.
- Let users point to local language-server and debugger executables. Any automatic installer must use the tool's official release source, explain what will be downloaded, verify published checksums/signatures where available, and require user approval before first installation or execution.
- Do not run workspace tasks or project scripts automatically when a folder is opened.

### Stage 2: declarative packs

Accept reviewed, schema-validated packages that contain data only:

- Theme and icon-theme JSON.
- Snippets and keymaps.
- Language identifiers, file associations, bracket pairs, and indentation rules.
- Syntax definitions only where the package format can be consumed without executing extension code.

Open VSX can be queried for candidate VS Code-style packages. DWCode must inspect the package manifest and license metadata, reject executable entry points (`main` or `browser`) in this stage, validate contributions against DWCode's supported schemas, and display the publisher, version, license, source, and requested data contributions before install. Install exact versions and preserve the package's origin and checksum in a local lock record.

Every accepted contribution must be presented in DWCode's visual identity. The Cybercore shell, typography, spacing, motion, and interaction patterns remain DWCode-owned. Imported color themes are mapped into the DWCode theme schema: the editor syntax palette may be adapted from the source theme, while application chrome retains DWCode surfaces, borders, and accent rules. Imported file and product icons are normalized for DWCode sizing, contrast, and restrained color use. Contribution views and settings use DWCode UI components. Do not silently alter and redistribute upstream assets: where adaptation is not permitted by the package license, use the package only as design inspiration and recreate original DWCode assets, with attribution when required. Keep the source package, license, attribution, and adaptation notes in the local package record.

### Stage 3: DWCode-native extensions

Define a versioned DWCode API and package manifest before executing third-party code. Start from narrow contributions—commands, menus, status items, providers, and settings—behind stable capability interfaces. Keep extension UI within DWCode-owned surfaces; do not allow extensions to replace the app shell wholesale.

Executable extensions require explicit installation and workspace/user trust, a separate host process, capability-scoped RPC, resource limits, cancellation, clear permissions, and an uninstall/disable path. No extension receives arbitrary filesystem or process access by default. Do not silently load code from an Open VSX VSIX as a native DWCode extension.

For the first native packages, publish a curated catalog — reviewed package by package, but at real scale rather than a handful of packs: the FOSS acceptance backlog in [FOSS_EXTENSION_REPOSITORY.md](FOSS_EXTENSION_REPOSITORY.md) already lists roughly 70 licence-checked packages across 16 ecosystems (Python, Java/Maven/Gradle, Android, GitLab, Google/Dart, Rust, Markdown, lint and format, SQL/NoSQL, Docker, GraphQL, Lua, Bash, CMake, Julia, Kotlin), and `npm run catalog:scan` re-gates every id in that file against the live registry before each release (`tools/openvsx-scan.mjs`, with hand-verified upstream licences in `tools/openvsx-verified.json`). Publish the index on GitHub Releases as a signed or checksum-pinned JSON document, grow it ecosystem by ecosystem behind the same licence gate, and revisit a hosted registry only when there are enough independent extensions to justify operating one.

## Compatibility and trust rules

- Keep DWCode API compatibility separate from Monaco's API and from VS Code's extension API.
- Provide a compatibility report per package; state whether it is declarative-only, needs a language server, or requires a future DWCode host.
- Show the package license and publisher before install; never assume Open VSX hosting changes the license.
- Pin exact versions and make updates opt-in, reviewable, and reversible.
- Do not fetch extensions from user-provided URLs by default. If local VSIX import is added, apply the same manifest, license, integrity, and compatibility checks.
- Treat workspace configuration and extension recommendations as untrusted until the user approves them.

## Proposed versioned contract

Keep the package format, contribution schema, and executable API version separate so each can evolve without disguising a breaking change.

```json
{
  "manifestVersion": 1,
  "id": "cybercore.example-tools",
  "version": "1.2.0",
  "engines": { "dwcode": ">=0.1.0 <1.0.0" },
  "dwcodeApi": "1.0",
  "kind": "declarative",
  "license": "MIT",
  "contributes": {
    "commands": [],
    "snippets": {},
    "themes": [],
    "languages": []
  },
  "capabilities": []
}
```

- `manifestVersion` is an integer for the package schema. A new incompatible schema gets a new integer and the loader rejects unknown versions with a useful explanation.
- `engines.dwcode` is a SemVer range for the app. `dwcodeApi` is the major/minor API contract. Patch releases add fixes only; minor releases may add optional APIs; major releases may remove or change APIs. Extensions declare the range they support, and DWCode checks it before activation.
- The host sends its supported API versions and capabilities during a JSON-RPC handshake. The extension must negotiate a supported version before it can activate. Unknown methods and capabilities fail closed.
- API stability levels are `proposed`, `preview`, and `stable`. Only stable APIs are promised across minor releases. Preview APIs require explicit opt-in and may change between releases.
- Initial executable API scope, after the host exists: register commands and keybindings, read settings, show notifications, contribute a status item, and register cancellable language providers. Workspace file reads/edits, terminals, processes, network, and arbitrary webviews are excluded from the first executable API release.

## Proposed capability permissions

Use named, versioned capabilities instead of broad “trusted extension” switches. The manifest declares requested capabilities; DWCode grants them per user and optionally per workspace. An install prompt lists the exact requested rights, the publisher, version, license, and package hash. Updates that add capabilities require consent again. Users can inspect and revoke grants from Settings; disabling or uninstalling an extension stops its host process and removes its contributions.

| Capability | Default | Rule |
| --- | --- | --- |
| `commands.register` | Allowed for reviewed declarative commands | Commands are namespaced by extension id; no shell execution. |
| `settings.read` | Allowed for extension-scoped settings | Other user settings need a separate grant. |
| `ui.statusItem` | Allowed after review | DWCode controls placement, text size, and click actions. |
| `workspace.read` | Denied | Grant per workspace; expose scoped document/folder handles, not arbitrary absolute paths. |
| `workspace.edit` | Denied | Ask before first edit; all edits flow through DWCode's undo/dirty-document model. |
| `terminal.create` | Denied | Separate consent and workspace trust; never grant implicitly from opening a project. |
| `process.exec` | Denied | Separate consent, executable allowlist, argument display, and confirmation for each new executable. |
| `network.connect` | Denied | Explicit opt-in with host allowlist; extension install alone grants no network. |
| `ui.webview` | Denied | Add only after a reviewed, origin-restricted UI bridge exists. |

Executable code runs out-of-process and talks to the editor through a capability broker over JSON-RPC. The broker checks every request, applies cancellation and resource limits, and never passes raw Qt/QML objects or unrestricted filesystem handles across the boundary. A workspace can disable all executable extensions. If the platform cannot provide the promised process isolation, executable extensions remain disabled there; declarative data packs continue to work.

This is a design proposal, not a security claim: before executable plugins ship, threat-model the host and test filesystem, process, network, resource-exhaustion, and workspace-trust boundaries on each supported platform.

## Sources

- Eclipse Foundation [Open VSX Registry FAQ](https://www.eclipse.org/legal/open-vsx-registry-faq/): Open VSX is a community-driven, open registry and provides license metadata for each extension.
- VS Code documentation [Extension Host](https://github.com/Microsoft/vscode-docs/blob/main/api/advanced-topics/extension-host.md): extensions run in local Node.js, web worker, or remote hosts with different capabilities.
- VS Code documentation [Web Extensions](https://github.com/microsoft/vscode-docs/blob/main/api/extension-guides/web-extensions.md): declarative-only contributions such as themes, grammars, and snippets are distinct from executable extensions.
- VS Code [FAQ](https://code.visualstudio.com/Docs/supporting/faq): Microsoft says the Visual Studio Marketplace is for the Visual Studio family of products and services.
