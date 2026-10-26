# Developer profile and DWCode priorities

This inventory was read from the installed VS Code CLI and local VS Code profile on 2026-10-21. It describes extensions installed for the profile; the available session did not expose which specific window is on desktop workspace 18, so it cannot prove that every listed extension is active in that window.

> **Superseded for planning:** [`FOSS_EXTENSION_SHORTLIST.md`](FOSS_EXTENSION_SHORTLIST.md) narrows
> this raw inventory with a popularity sweep of the top of the Open VSX registry, verified FOSS
> licences, per-area coverage (needs, Rust, C/C++, Git, syntax highlighting, HTML/CSS), and a tier
> per package saying whether it ships as a declarative pack, a native integration, or a future
> host-gated extension. This file remains the record of the original profile read.

## Current visual setup

- Active color theme: **Dark 2026** (VS Code built-in theme).
- Active file icon theme: **Material Icon Theme**.
- Also installed: **GitHub Color Theme**, with GitHub Dark Default, Dark Dimmed, Dark High Contrast, Dark Colorblind, Light Default, Light, Light High Contrast, and Light Colorblind variants.
- Also installed: **VSCode Icons** and Visual Studio C/C++ themes. Material Icon Theme is selected.

DWCode should keep its Cybercore theme as the default identity, ship a matching light and high-contrast variant, and support safe theme/icon-theme data packs so familiar choices can be adapted when their schemas and licenses check out. Imported packs must be restyled to DWCode's visual system: retain DWCode's shell, map source syntax colors into DWCode's theme tokens, and normalize icons to DWCode sizing and restrained contrast. Preserve upstream attribution and license terms; if a license does not allow asset modification or redistribution, create original DWCode assets inspired by the broad design cues instead of bundling the upstream files.

## Installed extension inventory

### Daily editor quality

- EditorConfig, Prettier, ESLint, Code Spell Checker, Indent Rainbow, Path IntelliSense, AutoDocstring, and Rainbow CSV.
- Markdown All in One, Jinja, JavaScript Snippets, ES7 React/Redux snippets, and Vetur.
- Code Runner, Live Server (two separately published extensions are installed), and PDF Viewer.

DWCode priorities: EditorConfig, formatting, diagnostics, spell checking, language snippets, Markdown, CSV viewing, and local preview. Use explicit formatter/linter configuration and project trust. Keep code execution behind run profiles and a visible trust prompt rather than running files on open. Provide one built-in preview server instead of requiring two competing Live Server extensions.

### Languages and debugging

- Python: Python, Pylance, Debugpy, Python Environments, Python Environment Manager, Python Extension Pack, Python Indent, and Django.
- C/C++: C/C++ extension pack, C/C++ DevTools, CMake Tools, and C/C++ themes.
- Go: Go.
- Dart and Flutter: Dart Code and Flutter.
- Java: Maven for Java.
- Web: Tailwind CSS, JavaScript/React snippets, Vetur, and Jinja.

DWCode priorities: LSP first for completion, diagnostics, navigation, and formatting; DAP after the document/task model is stable. Prioritize Python, C/C++, JavaScript/TypeScript, Go, Dart, HTML/CSS, and Java integrations. Keep language-server/debugger installation explicit, show the executable and source, and ask before installing or running tools.

### Source control, containers, and remote work

- Git History, Git Graph, and GitHub Pull Requests.
- Docker, Containers, Remote Containers, Remote SSH Edit, and Remote Explorer.
- CMake Tools also supports the native build workflow.

DWCode priorities: built-in Git status/diff/history and GitHub pull-request links; container and SSH workflows should be later opt-in integrations with explicit connection and workspace permissions. Do not treat a container extension as permission to start arbitrary project containers.

These are feature priorities based on the profile. They are not a promise to install or activate the corresponding VSIX packages. A future Open VSX inspector must show each package's publisher, license, requested contributions, and provenance before DWCode accepts it.

### AI tools

- Claude Code, OpenAI ChatGPT, and OpenAI Codex Audio.

DWCode should use provider adapters and user-configured credentials for AI features. These VS Code extensions are not automatically compatible and should not be installed or executed inside DWCode.

## Installed package versions

```text
anthropic.claude-code 2.1.304
batisteo.vscode-django 1.15.0
bradlc.vscode-tailwindcss 0.16.0
christian-kohler.path-intellisense 2.10.0
dart-code.dart-code 3.144.0
dart-code.flutter 3.144.0
dbaeumer.vscode-eslint 3.0.34
donjayamanne.githistory 0.6.20
donjayamanne.python-environment-manager 1.2.7
donjayamanne.python-extension-pack 1.7.0
dsznajder.es7-react-js-snippets 4.4.3
editorconfig.editorconfig 0.18.2
esbenp.prettier-vscode 12.4.0
formulahendry.code-runner 0.12.2
github.github-vscode-theme 6.3.5
github.vscode-pull-request-github 0.170.0
golang.go 0.58.0
kevinrose.vsc-python-indent 1.21.0
mechatroner.rainbow-csv 3.24.1
mhutchie.git-graph 1.30.0
ms-azuretools.vscode-containers 2.5.3
ms-azuretools.vscode-docker 2.0.0
ms-python.debugpy 2026.6.0
ms-python.python 2026.10.0
ms-python.vscode-pylance 2026.4.1
ms-python.vscode-python-envs 1.40.0
ms-vscode-remote.remote-containers 0.469.0
ms-vscode-remote.remote-ssh-edit 0.87.0
ms-vscode.cmake-tools 1.24.42
ms-vscode.cpp-devtools 0.6.18
ms-vscode.cpptools 1.34.4
ms-vscode.cpptools-extension-pack 1.5.1
ms-vscode.cpptools-themes 2.0.0
ms-vscode.live-server 0.4.20
ms-vscode.remote-explorer 0.5.0
njpwerner.autodocstring 0.6.1
octref.vetur 0.37.3
oderwat.indent-rainbow 8.3.1
openai.chatgpt 26.1007.21434
openai.codex-audio 26.1007.21434
pkief.material-icon-theme 5.39.0
ritwickdey.liveserver 5.7.10
streetsidesoftware.code-spell-checker 4.10.0
tomoki1207.pdf 1.2.2
visualstudioexptteam.intellicode-api-usage-examples 0.2.9
visualstudioexptteam.vscodeintellicode 1.3.2
vscjava.vscode-maven 0.46.0
vscode-icons-team.vscode-icons 13.0.0
wholroyd.jinja 0.0.10
xabikos.javascriptsnippets 1.8.0
yzhang.markdown-all-in-one 3.6.3
```

## Suggested DWCode rollout

1. **First release:** Cybercore Dark/Light/High Contrast themes, Material-style file icons, EditorConfig, formatting, snippets, Markdown, spell checking, CSV preview, and safe local web preview.
2. **Language tooling:** JavaScript/TypeScript and Python LSP; then C/C++, Go, Dart, and Java. Add DAP as the run/task model matures.
3. **Workflow:** Git history/diffs and GitHub links, then opt-in containers/SSH integrations.
4. **Extension support:** use Open VSX only as a source for reviewed declarative packages first. Offer a clear compatibility badge and do not promise compatibility for executable VS Code extensions until DWCode has its own isolated extension host and permission broker.

The inventory is a feature signal, not authorization to copy or redistribute third-party extension code. Any package DWCode imports still needs license, integrity, schema, and compatibility review.
