# DWCode FOSS extension repository

**Verified:** 2026-10-25 against the live Open VSX registry API (`/api/<ns>/<name>` for license and
presence, `/api/-/search?orderBy=downloadCount&sortOrder=desc` for a full popularity sweep of the
registry's top 80 of 20,163 extensions) and the GitHub license API for packages whose registry
license field was empty, a `SEE LICENSE IN …` pointer, or a placeholder. Extended 2026-10-26 in
two passes: an AI-agent sweep — `codex`, `antigravity`, `gemini`, `opencode`, `copilot` (40 results
each, licences re-checked upstream) — and an ecosystem sweep across **25 areas**: Python,
Java/Maven/Gradle, Android, GitLab, Google/Dart/Flutter, Rust, Markdown, lint & format, SQL/NoSQL,
Docker, GraphQL, Lua, Bash, CMake, Julia, Kotlin, YAML/JSON/XML, Terraform/Kubernetes/Helm,
TypeScript/Vue/Svelte/React, PHP, Ruby, C#/.NET, Elixir, test runners and debug adapters — see
[Ecosystem sweep](#ecosystem-sweep--2026-10-26) below.

**Gate result (2026-10-26):** `npm run catalog:scan` reports **206 ids checked · 179
catalog-eligible · 0 pending · 20 refused · 7 not on Open VSX**, exit 0. The 179 are listed in the
[appendix](#appendix--every-catalog-eligible-package-179); the refused and delisted ones are in
*Excluded*. Re-run `npm run catalog:scan` before each catalog release — licences and availability
change.

**Purpose:** decide which of the most-used free/open-source extensions DWCode should cover, and
*how*: as a declarative pack (blueprint M1), as a built-in integration DWCode implements natively
(EXTENSIONS.md Stage 1), or as a future executable extension behind the capability broker
(blueprint M2/M3+). It is also the review queue for the curated catalog (EXTENSIONS.md — every
package that enters the index is a row here first). It narrows and supersedes the raw inventory in
[USER_EXTENSION_PROFILE.md](USER_EXTENSION_PROFILE.md).

Selection rules applied:

- **Free *and* FOSS:** an OSI-approved (or equivalent) license identified in the package metadata
  or upstream repository. "Free tier" and "source available" do not qualify.
- **On Open VSX:** DWCode's policy sources only from Open VSX or reviewed first-party packs; the
  Visual Studio Marketplace is not a source. Marketplace-only packages are listed as out of reach
  unless their license permits repackaging as a DWCode pack.
- **DWCode-owned visuals:** imported themes/icons are mapped into DWCode tokens per
  [PACK_FORMAT.md](PACK_FORMAT.md); they are never dropped in raw.

Download counts below are Open VSX registry downloads (lifetime), used as a popularity signal only.

---

## Tier A — declarative-pack candidates (blueprint M1, available now)

Data-only, no executable entry point, validated by `tools/validate-packs.mjs`. These are the first
third-party packs DWCode should map.

| Extension | Open VSX id | License | Downloads | Contribution type |
| --- | --- | --- | --- | --- |
| Material Icon Theme | `PKief.material-icon-theme` | MIT | 6.55 M | icon theme |
| vscode-icons | `vscode-icons-team.vscode-icons` | MIT | 1.59 M | icon theme |
| One Dark Pro | `zhuangtongfa.material-theme` | MIT | 0.96 M | color theme |
| Dracula | `dracula-theme.theme-dracula` | MIT | (registry) | color theme |
| Gruvbox Material | `sainnhe.gruvbox-material` | MIT | (registry) | color theme |
| GitHub Theme | `GitHub.github-vscode-theme` | MIT | (registry) | color theme |
| ES7+ React/Redux/GraphQL snippets | `dsznajder.es7-react-js-snippets` | MIT | (registry) | snippets |
| JavaScript (Node) snippets | `xabikos.JavaScriptSnippets` | MIT | (registry) | snippets |
| VSCodeVim | `vscodevim.vim` | MIT | 1.98 M | keymap (declarative subset only) |

Notes:

- **VSCodeVim is a hybrid.** Its `contributes.keybindings` and keyboard layout data are
  declarative, but its modal editing engine is executable code. DWCode's Helix-inspired modal mode
  is built in; only the declarative keybinding contribution is pack material, and only if the
  package can be split without executing code (EXTENSIONS.md Stage 2 rule). Otherwise it is
  Stage 3 material.
- **Marketplace-only, therefore out of scope as packages:** `Catppuccin.catppuccin-vscode` and
  `enkia.tokyonight` both return 404 from the Open VSX API. Their licenses (MIT upstream) would
  permit an original DWCode-authored pack *inspired by* the palette — recreation, not bundling,
  per the adaptation rules.
- Icon packs are validated today but DWCode's Explorer does not render per-file icons yet; the
  compatibility note in the EXTENSIONS settings section should say so rather than implying support.

## Tier B — build in natively (Stage 1, no extension host needed)

The highest-traffic FOSS tooling. DWCode already owns the seams these extensions hook (save
lifecycle, diagnostics pipeline, settings funnel), so native integrations beat hosting a package
that assumes the VS Code API.

| Capability | Representative extension | License | Downloads | DWCode path |
| --- | --- | --- | --- | --- |
| Formatting | Prettier `esbenp.prettier-vscode` | MIT | 9.65 M | format-on-save wired to the existing save path; point at a project-local binary |
| Linting | ESLint `dbaeumer.vscode-eslint` | MIT | 6.42 M | diagnostics via the existing `problems` pipeline (today: LSP; ESLint as a second source) |
| EditorConfig | `EditorConfig.EditorConfig` | MIT | 2.31 M | apply `.editorconfig` on open in `WorkspaceBackend` (indent/charset/final newline) |
| Spell checking | cSpell `streetsidesoftware.code-spell-checker` | **GPL-3.0-or-later** | 2.17 M | mark **copyleft**: fine as an external process/config consumer; do not link into DWCode binaries |
| Markdown lint | `DavidAnson.vscode-markdownlint` | MIT | 1.84 M | reuse its rule config; feed results into the problems panel |
| CSV/TSV colorizing | `mechatroner.rainbow-csv` | MIT | 3.36 M | declarative language contribution in a pack (Tier A mechanism) + built-in CSV preview |
| Indent guides | `oderwat.indent-rainbow` | MIT | 0.59 M | Monaco decoration settings — already covered by DWCode's guides/whitespace preferences |
| Path completion | `christian-kohler.path-intellisense` | MIT | 0.57 M | Monaco completion provider over the workspace file list DWCode already has |
| Inline diagnostics | `usernamehw.errorlens` | MIT | 1.36 M | Monaco view zones/settings; check against DWCode's current-line highlight work |
| Markdown authoring | `yzhang.markdown-all-in-one` | MIT | (registry) | built-in Markdown preview + table of contents, one implementation instead of N |
| Git history | `donjayamanne.githistory` | MIT | (registry) | folded into DWCode's own Git work (blueprint task 3) |
| Live preview | `ritwickdey.LiveServer` | MIT | 3.09 M | **one built-in preview server** (profile decision), not a competing extension |
| Run file | `formulahendry.code-runner` | MIT (repo, GitHub API) | 1.05 M | DWCode's command runner with explicit trust prompt; never auto-run on open |
| Git graph | `mhutchie.git-graph` | *verify* — registry lists no license and the GitHub API returned 404 for `mhutchie/git-graph` | (registry) | do not adopt until license and repository status are confirmed |

## Tier B2 — language tooling verified in the top-80 sweep

Everything below is FOSS **and** on Open VSX, confirmed by the popularity sweep. These map to
Stage 1 work (DWCode's own LSP/DAP integration, "point at a local server"), not to hosting a
package — DWCode already runs `typescript-language-server`, `clangd`, `rust-analyzer`, and
`pyright-langserver` directly.

| Extension | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| Pyrefly (`meta.pyrefly`) | MIT | 92.5 M | alternative Python LSP; offer beside `pyright-langserver` in the language-server picker |
| Ruby LSP (`Shopify.ruby-lsp`) | MIT | 44.5 M | add `ruby-lsp` to the known-server table |
| Go (`golang.Go`) | MIT | 42.9 M | `gopls` on `PATH` (already documented pattern) |
| Project Manager/Maven/Gradle/Test for Java (`vscjava.*`) | **MIT on all four — verified**: registry says `SEE LICENSE IN` / empty for debug, gradle and test, so each upstream `LICENSE` was read (`microsoft/vscode-{java-debug,gradle,java-test}`, all MIT). `vscjava.vscode-java-pack` is a metapackage with no code of its own | 23–41 M | only the Java language server itself is needed; DWCode consumes `jdtls` output directly |
| clangd (`llvm-vs-code-extensions.vscode-clangd`) | MIT | 31.5 M | DWCode already launches `clangd`; no package needed |
| rust-analyzer | MIT OR Apache-2.0 | 7.4 M | already the documented Rust path |
| Python (`ms-python.python`) | MIT (repo) | 61.6 M | `pyright-langserver` already bundled-adjacent; **Pylance stays excluded** |
| Ruff (`charliermarsh.ruff`) | MIT | 3.9 M | lint/format via one binary; feed diagnostics into the problems panel |
| BasedPyright (`detachhead.basedpyright`) | MIT | 2.9 M | opt-in alternative type checker for Python |
| YAML (`redhat.vscode-yaml`) | MIT | 8.9 M | schema-driven validation → problems panel (schema fetch needs `network.connect`) |
| XML (`redhat.vscode-xml`) | EPL-2.0 | 1.6 M | `lemminx` language server on `PATH` |
| markdownlint | MIT | 1.8 M | (Tier B) rule config reused natively |
| Markdown Preview Enhanced (`shd101wyy.markdown-preview-enhanced`) | **NCSA confirmed** — registry `NCSA`, upstream `LICENSE.md` carries the University of Illinois/NCSA text (re-checked 2026-10-26) | 2.49 M | candidate for DWCode's single built-in preview instead of N preview extensions |
| SonarLint (`SonarSource.sonarlint-vscode`) | **LGPL-3.0** (repo) | 2.1 M | copyleft: run as an external process with its own config, never link statically |
| CMake Tools (`ms-vscode.cmake-tools`) | MIT | 2.3 M | task/run profiles (blueprint task 4) |
| Terraform (`hashicorp.terraform`) | MPL-2.0 | 2.4 M | `terraform-ls` on `PATH`; MPL file-level copyleft is fine for an external server |
| Dart / Flutter (`Dart-Code.*`) | MIT (repo; registry says `SEE LICENSE IN LICENSE`) | 5.1 M / 4.7 M | `dart language-server` on `PATH` |
| CodeLLDB (`vadimcn.vscode-lldb`) | MIT | 1.4 M | DAP adapter candidate once DAP lands |
| JavaScript Debugger (`ms-vscode.js-debug`) | MIT | 3.2 M | DAP adapter for Node; the DAP client is DWCode's own |
| Jupyter (`ms-toolsai.jupyter` + renderers) | MIT | 11.1 M / 10.5 M | notebook UI is a large DWCode feature — decide before committing |
| GitHub Pull Requests / GitHub Actions (`GitHub.*`) | MIT | 6.6 M / 4.5 M | need `network.connect` + auth storage; belongs to the Git milestone, likely built in |
| Container Tools (`ms-azuretools.vscode-containers`) | **MIT confirmed** — registry `SEE LICENSE IN`, upstream `microsoft/vscode-containers` `LICENSE.md` is MIT (re-checked 2026-10-26). The older Docker extension `ms-azuretools.vscode-docker` (7.13 M) is MIT on the same evidence | 14.8 M | Docker interaction is `process.exec` territory — one native container panel instead of two extensions |
| Intelephense (`bmewburn.vscode-intelephense-client`) | **custom `SEE LICENSE IN` with no public repo — treat as non-FOSS** | 2.2 M | excluded |

## Coverage by area — Rust, C/C++, Git, syntax highlighting, HTML/CSS

Swept per keyword on the registry (top results by downloads with license lookup), so the
"most used" claim is checked rather than assumed.

### Rust

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| rust-analyzer (`rust-lang.rust-analyzer`) | MIT OR Apache-2.0 | 7.4 M | already the documented Rust path — `rust-analyzer` on `PATH` |
| `rustfmt` / `clippy` via `cargo` | MIT/Apache-2.0 (toolchain) | — | format-on-save and diagnostics through the same external-binary pattern as Prettier/Ruff |
| Even Better TOML (`tamasfe.even-better-toml`) | **MIT confirmed** — registry `SEE LICENSE IN LICENSE.md`, upstream `tamasfe/taplo` root `LICENSE` is MIT (re-checked 2026-10-26) | 693 K | `cargo.toml`/`Cargo.lock` parsing; Tombi (`tombi-toml.tombi`, MIT, 548 K) is the FOSS language-server alternative |
| Rust Language Basics (`vscode.rust`) | VS Code package license | 316 K | TextMate grammar — see *syntax highlighting* below; Monaco already ships Rust highlighting via `basic-languages` |

### C and C++

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| clangd (`llvm-vs-code-extensions.vscode-clangd`) | MIT | 31.5 M | **already supported**: DWCode launches `clangd` directly, no package needed |
| Clang-Format (`xaver.clang-format`) | MIT (registry); upstream repo lookup 404 — verify path | 81 K | external `clang-format` binary for format-on-save |
| C/C++ Compile Run (`danielpinto8zz6.c-cpp-compile-run`) | **GPL-3.0** | 109 K | copyleft, and DWCode's task/run profiles cover it natively |
| C/C++ Runner (`franneck94.c-cpp-runner`) | MIT | 333 K | same: fold into task profiles rather than hosting |
| C/C++ Debug (`KylinIdeTeam.cppdebug`) | MIT | 482 K | DAP/lldb path alongside CodeLLDB when DAP lands |
| Microsoft C/C++ (`ms-vscode.cpptools`) | **not on Open VSX (404)** | — | out of reach as a package; `clangd` is the FOSS-equivalent path DWCode already takes |

### Git

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| GitLens (`eamodio.gitlens`) | `NOASSERTION` — source-available freemium | 18.3 M | **excluded**; the useful subset (blame, history, diff) belongs to DWCode's own Git milestone |
| Git (`vscode.git`) | VS Code package license | 354 K | Microsoft's own Git integration — reference only; DWCode implements status/diff/commit natively (blueprint task 3) |
| Git Blame (`waderyan.gitblame`) | MIT | 614 K | native: status-bar blame line fed by `git blame` |
| Git Graph (`mhutchie/vscode-git-graph`) | **repo reports `NOASSERTION`** — no license file | (registry) | do not adopt; DWCode's diff view covers the need |
| Git History (`donjayamanne.githistory`) | MIT (registry) | (registry) | native history view in the Git milestone |
| GitHub Pull Requests / Actions (`GitHub.*`) | MIT | 6.6 M / 4.5 M | `network.connect` + auth; likely built-in, not hosted |
| GitLab Workflow | MIT | 2.9 M | Tier C |

### Syntax highlighting

- **Monaco already ships grammar coverage** for JS/TS, JSON, CSS, HTML, Markdown, Rust, Python,
  and the rest through `basic-languages` — the same ground the `vscode.<lang> Language Basics`
  packages cover on VS Code (all 296–407 K downloads, VS Code package license). No work needed.
- **Third-party grammars** (TextMate `.tmLanguage.json`) are the Stage 2 declarative case:
  EXTENSIONS.md allows syntax definitions only when consumed without executing code. Monaco does
  not consume TextMate directly, so DWCode needs a **tmLanguage → Monarch converter** — which now
  exists as [`tools/tm2monarch.mjs`](tools/tm2monarch.mjs) (zero dependencies, deterministic
  output, `--self-test` runs 16 conversion cases *and* feeds every emitted definition through
  Monaco's own `monarchCompile.js`, so a bad grammar fails in CI rather than at load time):
  `node tools/tm2monarch.mjs grammar.tmLanguage.json -o lang.json`. Verified end-to-end on real
  grammars — Red Hat's YAML (40 states, 18 root rules) and VS Code's Markdown (the unsupported
  `while` construct is reported and dropped, never silently). Still outstanding is the other half
  of that bullet: extending `validate-packs.mjs` to check a *converted* grammar inside a pack
  (state names resolve, `@pop` balance, token ids exist in the theme schema).
- Grammar-adjacent linters that are FOSS: **Harper** (`elijah-potter.harper`, Apache-2.0, 110 K),
  **LTeX+** (`ltex-plus.vscode-ltex-plus`, MPL-2.0, 21 K), **LanguageTool Linter**
  (`davidlday.languagetool-linter`, Apache-2.0, 24 K) — all external-process or server-based →
  Stage 1/3 depending on whether DWCode drives them as plain processes.
- `fabiospampinato.vscode-highlight` (regex text highlighting): registry lists no license, GitHub
  says MIT — verify at catalog time.

### HTML and CSS

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| Stylelint (`stylelint.vscode-stylelint`) | MIT | 746 K | external `stylelint` binary → problems panel (same pattern as ESLint) |
| HTML CSS Support (`ecmel.vscode-html-css`) | MIT (repo confirmed) | 790 K | class/id completion from linked CSS — Monaco completion provider over workspace files |
| CSS Peek (`pranaygp.vscode-css-peek`) | MIT (repo confirmed) | 180 K | go-to-definition across HTML↔CSS; native provider |
| CSS Navigation (`pucelle.vscode-css-navigation`) | MIT | 53 K | same mechanism |
| Live Sass Compiler (`glenn2223.live-sass`) | MIT (registry); repo lookup 404 — verify | 108 K | task profile running `sass --watch`, with the run-profile trust prompt |
| html-validate (`html-validate.vscode-html-validate`) | MIT (registry); repo path verify | 370 K | external binary → problems panel |
| Rainbow HTML (`wumbl3.rainbow-html`) | MIT | 14 K | native: Monaco decorations, no package needed |
| Tailwind CSS IntelliSense | MIT | 2.6 M | Tier C (`workspace.read` + its language server) |
| HTML/CSS Language Basics (`vscode.html`, `vscode.css`) | VS Code package license | 407 K / 402 K | already bundled with Monaco |
| Black Formatter (`ms-python.black-formatter`), isort | MIT | 1.5 M / 908 K | external binaries on the format-on-save path (cross-language, worth listing here) |

## Ecosystem sweep — 2026-10-26

Requested areas checked one keyword and one package id at a time against the live registry, then
upstream wherever the `license` field was a pointer. **Every row is FOSS and on Open VSX** — the
packages that failed the gate are in *Excluded* below or in the per-ecosystem note. Hand-checked
upstream readings are recorded in [`tools/openvsx-verified.json`](tools/openvsx-verified.json) so
`npm run catalog:scan` can re-check this whole section in one command.

### Python

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| Python Debugger (`ms-python.debugpy`) | MIT | 62.6 M | `debugpy` is the DAP adapter — wire it up when DAP lands (M3); DWCode owns the client |
| Python Environments (`ms-python.vscode-python-envs`) | MIT (registry field **empty**; upstream `microsoft/vscode-python-environments` read and confirmed) | 45.2 M | interpreter/venv discovery → DWCode's language-server picker and an "interpreter" setting |
| Python Indent (`KevinRose.vsc-python-indent`) | MIT | 0.34 M | continuation-indent behaviour → Monaco indentation rules, a preference not a package |
| autoDocstring (`njpwerner.autodocstring`) | MIT | 0.35 M | docstring template → Tier A snippet pack, or one native command |

Already covered above: Pyrefly, `ms-python.python`, Ruff, BasedPyright, Black, isort, Jupyter.
**Pylance stays excluded** — proprietary.

### Java, Maven, Gradle

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| Language Support for Java (Red Hat) (`redhat.java`) | EPL-2.0 | 46.9 M | `jdtls` on `PATH` — the one Java server DWCode needs |
| Project Manager for Java (`vscjava.vscode-java-dependency`) | MIT | 41.5 M | project tree → DWCode explorer "project views" |
| Maven for Java (`vscjava.vscode-maven`) | MIT | 28.6 M | task profiles over `mvn` (blueprint task 4) + `network.connect` only for dependency fetch |
| Java Debug (`vscjava.vscode-java-debug`) | **MIT verified** (registry `SEE LICENSE IN` → upstream `LICENSE.txt`) | 27.9 M | the `java-debug` agent DWCode's DAP client would launch |
| Gradle (`vscjava.vscode-gradle`) | **MIT verified** (registry `SEE LICENSE IN` → upstream `LICENSE.md`) | 27.9 M | task profiles over `gradle` |
| Java Test (`vscjava.vscode-java-test`) | **MIT verified** (registry empty → upstream `LICENSE.txt`) | 27.8 M | JUnit runs → problems panel, same shape as the ESLint path |
| Extension Pack for Java (`vscjava.vscode-java-pack`) | MIT | 23.8 M | metapackage, no code of its own — reference only |
| MicroProfile (`redhat.vscode-microprofile`) | Apache-2.0 | 0.17 M | niche; skip unless asked |

### Android

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| Android (`adelphes.android-dev-ext`) | MIT | 39 K | `adb`/`logcat`/`am start` behind `process.exec` — a task/run profile, not a hosted package |
| Android Emulator (`duchuynguyen.android-emulator`), DevKit (`pavi2410.android-devkit`), ADB WLAN (`HanWang.android-adb-wlan`), AVD Viewer (`D0Tech.vs-avd`) | MIT | 1–10 K each | the same adb helpers; DWCode's task profiles cover them natively |
| Layout XML (`*.xml` for `res/layout`) | — | — | Monaco already opens them; `aapt`/`apkanalyzer` from the SDK (Apache-2.0) as external processes |

Note: there is **no high-download Android extension on Open VSX** — Android Studio's tooling ships
inside the IDE, not the registry. The top hits by downloads (`DiemasMichiels.emulate`,
`android-ios-layoutcodegen.*`, `krishna-kudari.android-studio-lite`) all report `SEE LICENSE IN` or
an empty field and are excluded on the gate, not on usefulness.

### GitLab

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| GitLab Workflow (`GitLab.gitlab-workflow`) | MIT | 2.9 M | `network.connect` with a `gitlab.com` allowlist + token storage — Tier C, or built into the Git milestone |
| gitlab-ci-ls (`alesbrelih.gitlab-ci-ls`) | MIT | 1.7 K | `gitlab-ci-ls` on `PATH` → YAML diagnostics for `.gitlab-ci.yml` |
| Git Web Links (`reduckted.vscode-gitweblinks`) | MIT | 30 K | permalink/permalink-to-line builder — trivially native in the Git milestone |
| Git Blame (`waderyan.gitblame`) | MIT | 0.61 M | status-bar blame line fed by `git blame` (Tier B) |

### Google, Dart, Flutter, Go

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| Go (`golang.Go`) | MIT | 42.9 M | `gopls` on `PATH` (Tier B2) |
| Dart / Flutter (`Dart-Code.Dart-Code`) | **MIT verified** (registry `SEE LICENSE IN LICENSE` → upstream `LICENSE`) | 5.97 M | `dart language-server` on `PATH`; `flutter` CLI as a task profile |
| Container Tools (`ms-azuretools.vscode-containers`) | **MIT verified** | 14.8 M | docker/podman CLI → one native container panel |
| Docker (`ms-azuretools.vscode-docker`) | **MIT verified** (upstream `microsoft/vscode-docker`) | 7.13 M | same — see Docker below |
| Docker DX (`docker.docker`, official `docker` publisher) | Apache-2.0 | 269 K | same; `docker/vscode-extension` upstream |
| Google Workspace MCP (`mercurial.google-workspace-mcp`) | MIT | 0.12 M | MCP server as an external process → needs `network.connect` + explicit consent (Tier C) |

### Rust

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| rust-analyzer (`rust-lang.rust-analyzer`) | MIT OR Apache-2.0 | 7.4 M | already the documented Rust path — `rust-analyzer` on `PATH` |
| crates (`serayuzgur.crates`) | **MIT verified** (registry `SEE LICENSE IN` → upstream `LICENSE`) | 155 K | crates.io version completion — needs `network.connect`, so Tier C |
| Even Better TOML (`tamasfe.even-better-toml`) | **MIT verified** | 693 K | `cargo.toml`/`Cargo.lock`; Tombi (MIT, 548 K) is the LS alternative |
| CodeLLDB (`vadimcn.vscode-lldb`) | MIT | 1.4 M | DAP adapter once DAP lands |
| `rustfmt` / `clippy` via `cargo` | MIT/Apache-2.0 (toolchain) | — | format-on-save and diagnostics through the Prettier/Ruff external-binary pattern |

### Markdown

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| Markdown Preview Enhanced (`shd101wyy.markdown-preview-enhanced`) | **NCSA confirmed** upstream | 2.49 M | front-runner for DWCode's single built-in preview |
| Markdown Preview Mermaid (`bierner.markdown-mermaid`) | MIT | 0.88 M | Mermaid rendering inside the preview — in a sandboxed webview, never `process.exec` |
| Cherry Markdown (`cherryMarkdownPublisher.cherry-markdown`) | Apache-2.0 | 40 K | full WYSIWYG editor; a large feature decision, not a quick add |
| Markdown All in One, markdownlint | MIT | 0.9–1.8 M | already Tier B — preview + TOC and rule config, built in |

### Lint, format, spell

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| ShellCheck (`timonwong.shellcheck`) | MIT | 789 K | external `shellcheck` → problems panel (same pattern as ESLint) |
| todo-tree (`gruntfuggly.todo-tree`) | MIT | 491 K | native: DWCode's own workspace TODO/NOTE scan, no package |
| shell-format (`foxundermoon.shell-format`) | **MIT verified** (registry field empty → upstream `LICENSE`) | 143 K | `shfmt` on the format-on-save path |
| cmake-format (`cheshirekow.cmake-format`) | GPL-3.0-or-later | 152 K | external process only — copyleft kept at arm's length, never linked |
| typos (`tekumara.typos-vscode`) | MIT | 93 K | the Rust `typos` binary → problems panel; fast and offline |
| Bats (`jetmartin.bats`) | MIT | 5 K | shell test runner as a task profile |

Already Tier B: Prettier, ESLint, stylelint, EditorConfig, markdownlint, errorlens, indent-rainbow,
cSpell (GPL — external process), Harper (Apache-2.0).

### SQL and NoSQL

Database access is `process.exec` plus an explicit host allowlist, so all of these are Tier C — but
they are the *FOSS* set, which is what the catalog needs.

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| SQL Tools (`mtxr.sqltools`) | MIT (registry **and** upstream `mtxr/vscode-sqltools` `LICENSE`) | 1.13 M | client host for the drivers below |
| SQLTools drivers: PostgreSQL (`mtxr.sqltools-driver-pg`), MySQL (`-mysql`), SQLite (`-sqlite`) | MIT | 550 K / 110 K / 90 K | same host; each driver is a separate licensed package |
| MySQL Client (`cweijan.vscode-mysql-client2`) | **MIT verified** (registry empty → upstream `cweijan/vscode-database-client`) | 1.38 M | same capability, independent implementation |
| MongoDB (`mongodb.mongodb-vscode`) | **Apache-2.0 verified** (registry `SEE LICENSE IN` → upstream `LICENSE.txt`) | 493 K | `mongosh` as the process, not a bundled driver |
| SQL Server (`ms-mssql.mssql`) | **MIT verified** (registry `SEE LICENSE IN` → upstream `LICENSE.txt`) | 340 K | `sqlcmd` on `PATH` |
| SQLite (`alexevzz`/`alexcvzz.vscode-sqlite`) | Apache-2.0 | 170 K | `sqlite3` CLI |
| PostgreSQL (`ckolkman.vscode-postgres`) | MIT | 90 K | `psql` |
| MySQL (`formulahendry.vscode-mysql`) | MIT | 40 K | same |
| Prisma (`prisma.prisma`) | Apache-2.0 | 1.56 M | `prisma` CLI as a task profile |

Excluded here: SQLite Viewer (`qwtel.sqlite-viewer`, 1.02 M — PRO paywall), Redis for VS Code
(`Redis.redis-for-vscode` — SSPL-1.0), DBCode (`dbcode.dbcode` — `SEE LICENSE IN`).

### Docker and containers

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| Container Tools (`ms-azuretools.vscode-containers`) | **MIT verified** | 14.8 M | images/containers/logs via `docker`/`podman` on `PATH` |
| Docker (`ms-azuretools.vscode-docker`) | **MIT verified** | 7.13 M | same; fold both into one native panel |
| Docker DX (`docker.docker`) | Apache-2.0 | 269 K | official Docker publisher, same integration |
| Dev Containers / Remote (`ms-vscode.remote-containers`) | — | — | **not on Open VSX** — out of reach as a package; the *image* side is what DWCode can do |

### GraphQL

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| GraphQL: Syntax Highlighting (`GraphQL.vscode-graphql-syntax`) | MIT | 1.02 M | TextMate grammar → Tier A: the converter exists (`tools/tm2monarch.mjs`), so this is one conversion + validator pass away |
| GraphQL: Language Feature Support (`GraphQL.vscode-graphql`) | MIT | 584 K | `graphql-language-service` on `PATH` |
| Apollo (`apollographql.vscode-apollo`) | MIT | 40 K | schema checks as an external process |
| Inline Operation Execution (`GraphQL.vscode-graphql-execution`) | MIT | 20 K | needs `network.connect` → Tier C |

### Lua

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| Lua Language Server (`sumneko.lua` → `LuaLS/lua-language-server`) | MIT | 498 K | `lua-language-server` on `PATH` — the one Lua server worth having |

Game-specific Lua packages (`openra.vscode-openra-lua`, `TradeSkillMaster.wowlua-ls`, `qbox.qbx-lua`)
are GPL but tied to one engine — excluded on usefulness, not licence.

### Bash and shell

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| Bash IDE (`mads-hartmann.bash-ide-vscode`) | MIT | 199 K | it wraps `bash-language-server` — run the server directly |
| ShellCheck (`timonwong.shellcheck`) | MIT | 789 K | external binary → problems panel |
| shell-format (`foxundermoon.shell-format`) | **MIT verified** | 143 K | `shfmt` on format-on-save |
| Bash Debug (`rogalmic.bash-debug`) | MIT | 80 K | DAP adapter later |
| Shellscript LS (`juandiazan.bash-lsp`) | MIT | 4 K | alternative Bash server, same `PATH` pattern |

### CMake

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| CMake Tools (`ms-vscode.cmake-tools`) | MIT | 2.25 M | configure/build/test task profiles (already Tier B2) |
| CMake (`twxs.cmake`) | MIT | 431 K | grammar + commands → Tier A grammar, plus a task profile |
| cmake-format (`cheshirekow.cmake-format`) | GPL-3.0-or-later | 152 K | external process |
| CMake Test Explorer (`fredericbonnet.cmake-test-adapter`) | BSD-3-Clause | 10 K | test adapter → after DWCode has a test runner |

### Julia

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| Julia (`julialang.language-julia`) | **MIT verified** (registry `SEE LICENSE IN LICENSE` → upstream `LICENSE`) | 261 K | the Julia language server runs inside `julia` — start it as a process, JVM-free but slow to boot |
| Julia Formatter (`singularitti.vscode-julia-formatter`) | MIT | 10 K | `JuliaFormatter.jl` as a format-on-save target |
| Jupyter (`ms-toolsai.jupyter`) | MIT | 11.1 M | notebook UI is a big DWCode feature — decide before committing |

### Kotlin

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| Kotlin (`fwcd.kotlin`) | MIT | 734 K | `kotlin-language-server` on `PATH` (JVM — needs the M4 resource limits) |
| Kotlin Language (`mathiasfrohlich.Kotlin`) | Apache-2.0 | 70 K | TextMate grammar → Tier A via `tools/tm2monarch.mjs` (same one-pass conversion as GraphQL) |
| Kotlin LSP (`JetBrains.kotlin-server` → `Kotlin/kotlin-lsp`) | **Apache-2.0 verified** | 50 K | JetBrains' official server — prefer it over `fwcd` once M4 resource limits exist |
| ktfmt (`omBratteng.ktfmt-kotlin-formatter`) | BSD-3-Clause | 4 K | external `ktfmt` on format-on-save |

### Data formats — YAML, JSON, XML

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| YAML (`redhat.vscode-yaml`) | MIT | 8.88 M | schema-driven validation → problems panel (already Tier B2; schema fetch needs `network.connect`) |
| XML (`redhat.vscode-xml`) | EPL-2.0 | 1.64 M | `lemminx` on `PATH` (already Tier B2) |
| Schemastore (`remcohaszing.schemastore`) | MIT | 240 K | the schema catalogue behind both validators — one `network.connect` allowlist for `schemastore.org` |
| XML Tools (`DotJoshJohnson.xml`) | MIT | 220 K | formatter/XPath → native or an `xmlstarlet` task profile |
| Pretty XML (`PrateekMahendrakar.prettyxml`) | MIT | 120 K | format-on-save |
| Fix JSON (`oliversturm.fix-json`), JSON Crack (`AykutSarac.jsoncrack-vscode`) | MIT | 30 K each | parse-error repair and visualisation — both fold into one built-in JSON tool |
| YAML sort, JSON format, YAML doctor (`PascalReitermann93.vscode-yaml-sort`, `saltcoreyan.json-format-sortcore`, `darioajr.yaml-doctor`) | MIT / Apache-2.0 | 40 K each | external formatters on the format-on-save path |

### Infrastructure — Terraform, Kubernetes, Helm

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| Terraform (`hashicorp.terraform`) | MPL-2.0 | 2.4 M | `terraform-ls` on `PATH` (already Tier B2) |
| Kubernetes Tools (`ms-kubernetes-tools.vscode-kubernetes-tools`) | Apache-2.0 | 1.25 M | `kubectl`/`helm`/`docker` as external processes; the YAML navigation half is native Monaco |
| helm-ls (`helm-ls.helm-ls`) | MIT | 20 K | `helm-ls` language server on `PATH` |
| OpenShift Toolkit (`redhat.vscode-openshift-connector`) | MIT | 70 K | `oc` CLI as a task profile |
| Kubernetes CRD snippets (`tumido.crd-snippets`) | GPL-3.0-or-later | 10 K | snippet pack → Tier A (copyleft data, fine) |
| Terraform plan colours / graph (`lite2073.tfplan-colors`, `saramorillon.terraform-graph`) | MIT | 10 K each | `terraform show -json` parsing → one native plan viewer |

### Web frameworks — TypeScript, JavaScript, Vue, Svelte, React

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| JavaScript Debugger (`ms-vscode.js-debug`) | MIT | 3.21 M | Node/Chrome DAP adapter (already Tier B2) |
| Svelte (`svelte.svelte-vscode`) | MIT | 2.17 M | `svelte-language-server` on `PATH` |
| JS Debugger Companion, Profile Table (`ms-vscode.js-debug-companion`, `ms-vscode.vscode-js-profile-table`) | MIT | 2.1–2.2 M | DAP extras; the flame/table visualiser is a native DWCode view, not a package |
| ES7+ React/Redux snippets (`dsznajder.es7-react-js-snippets`) | MIT | 910 K | already Tier A |
| JS/TS Nightly (`ms-vscode.vscode-typescript-next`) | MIT | 680 K | ships the TS nightly — DWCode already embeds Monaco's own TypeScript, so this is reference only |
| Import Cost (`wix.vscode-import-cost`) | MIT | 110 K | native: bundle-size hints from the workspace module graph |
| Total TypeScript / ts-error-translator (`mattpocock.ts-error-translator`) | MIT | 60 K | native: TS error rewording, no extension needed |
| Vue (`znck.vue`) | MIT | 40 K | small Vue LS; `Vue.volar` (10.95 M, MIT upstream at `vuejs/language-tools`) is the one to adopt |
| Vue 2/3, React, Svelte snippet packs (`NicholasHsiang.*`, `fivethree.vscode-svelte-snippets`) | MIT | 10–60 K | Tier A snippet packs |

### PHP

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| PHP Debug (`xdebug.php-debug`) | MIT | 800 K | DAP adapter over Xdebug's `dbgp` protocol |
| PHPUnit & Pest Test Explorer (`recca0120.vscode-phpunit`) | MIT | 70 K | test runner → after DWCode has one |
| PHP Language Support (`AnessaEah.php-intelephense-language-support`) | MIT | 10 K | the FOSS stopgap while Intelephense stays non-FOSS |

PHP is the weakest ecosystem on Open VSX: `bmewburn.vscode-intelephense-client` (2.2 M) is excluded
(custom terms, no public repo), and no large FOSS PHP language server has taken its place. The DWCode
route is `phpactor` / `phpstan` / `composer` as external processes, plus `xdebug` for debugging.

### Ruby

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| Ruby LSP (`Shopify.ruby-lsp`) | MIT | 44.5 M | already Tier B2 — `ruby-lsp` on `PATH` |
| Solargraph (`castwide.solargraph`) | MIT | 39 K | alternative Ruby language server |
| Sorbet (`sorbet.sorbet-vscode-extension`) | Apache-2.0 | 60 K | `srb` type-checking as a task profile |
| Standard Ruby (`testdouble.vscode-standard-ruby`) | MIT | 8 K | `standardrb` → format-on-save |
| Ruby Test Explorer (`connorshea.vscode-ruby-test-adapter`) | MIT | 10 K | test adapter → after DWCode has one |

### C# and .NET

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| .NET Install Tool (`ms-dotnettools.vscode-dotnet-runtime`) | MIT | 2.14 M | downloads a .NET runtime (`network.connect`); DWCode targets the `dotnet` already on `PATH` instead |
| KIDE C# / Test Explorer (`kide.kide-csharp`, `kide.kide-test-explorer`) | MIT | 10–70 K | thin wrappers over `dotnet build` / `dotnet test` → task profiles |
| .NET Meteor (`nromanov.dotnet-meteor`), dotnet-start (`pavel-purma.dotnet-start`) | MIT | 10–40 K | run/debug profiles over `dotnet run` |

**`ms-dotnettools.csharp` and `ms-dotnettools.csdevkit` both return 404 from Open VSX** — Microsoft's
C# tooling is Marketplace-only, so it is out of reach as a package whatever its licence says. DWCode's
path is the .NET SDK itself (`dotnet build` / `test` / `run` as task profiles); re-check the licence of
any Roslyn language server before adopting it, since that codebase mixes licences.

### Elixir

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| ElixirLS (`elixir-lsp.elixir-ls`) | MIT | 234 K | `elixir-ls` on `PATH` (a Mix project — `mix deps.get` first) |
| Lexical (`lexical-lsp.lexical`) | Apache-2.0 | 10 K | newer, pure-Elixir language server |
| Expert (`ExpertLSP.expert`) | Apache-2.0 | 10 K | alternative |
| hex.pm IntelliSense (`benvp.vscode-hex-pm-intellisense`) | MIT | 3 K | package metadata → needs `network.connect` |

### Test runners

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| Vitest (`vitest.explorer`) | MIT | 870 K | official explorer → DWCode's test runner over `npx vitest` |
| Jest / Vitest Runner (`firsttris.vscode-jest-runner`) | MIT | 914 K | same shape: parse runner output into the problems panel |
| PHPUnit & Pest (`recca0120.vscode-phpunit`) | MIT | 70 K | |
| Ruby Test Explorer (`connorshea.vscode-ruby-test-adapter`) | MIT | 10 K | |
| Test Adapter Converter (`ms-vscode.test-adapter-converter`) | MIT | 160 K | a shim for VS Code's test API — no DWCode equivalent, reference only |

Every row here wants the same prerequisite: a test-runner command in DWCode with output parsing
(blueprint task 4). Then each is a binary plus a config, not a package.

### Debug adapters (DAP)

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| JavaScript Debug (`ms-vscode.js-debug`) | MIT | 3.21 M | Node/Chrome adapter |
| CDT GDB (`eclipse-cdt.cdt-gdb-vscode`) | EPL-2.0 | 1.0 M | the FOSS GDB adapter — pairs with DWCode's existing `clangd` path |
| PHP Debug (`xdebug.php-debug`) | MIT | 800 K | Xdebug `dbgp` |
| CodeLLDB (`vadimcn.vscode-lldb`) | MIT | 1.4 M | Rust/C/C++ (Tier B2) |
| CDT WebSocket Adapter (`eclipse-cdt.websocket-adapter`) | EPL-2.0 | 2 K | remote DAP transport |

DAP is one JSON-RPC protocol, so DWCode needs **one client** (blueprint M2/M3) and then every row
above is a binary on `PATH` — exactly the relationship it already has with language servers.

**What this changes about scale:** the sweep brings the catalog-eligible total to **179 packages
across 25 ecosystems and capability areas** — every one licence-checked and present on Open VSX.
That is the backlog for the curated catalog, the reason EXTENSIONS.md no longer calls it "small",
and the reason the licence gate is a script (`tools/openvsx-scan.mjs`) instead of a person reading
registry pages by hand. The complete list is in the [appendix](#appendix--every-catalog-eligible-package-179)
at the end of this file, regenerated with `node tools/openvsx-scan.mjs --doc --appendix`.

## Tier C — requires the executable host (blueprint M2/M3+)

FOSS, but they execute code and/or spawn processes, so they wait for the capability broker.

| Extension | License | Downloads | What DWCode needs first |
| --- | --- | --- | --- |
| Continue | Apache-2.0 | (registry) | `network.connect` with host allowlist, `process.exec` for model runtimes, explicit user consent |
| Cline (`saoudrizwan.claude-dev`) | Apache-2.0 | (registry) | same as Continue, plus per-workspace trust; upstream repo `cline/cline` is Apache-2.0 (verify the published package matches) |
| Tailwind CSS IntelliSense | MIT | 0.66 M | `workspace.read` + bundled/external language server on `PATH` |
| Kilo Code (`kilocode.kilo-code`) | MIT (registry) — *upstream repo path unconfirmed at time of writing, verify* | 4.6 M | agent needs `network.connect`, `process.exec`, and workspace trust |
| GitLab Workflow (`GitLab.gitlab-workflow`) | MIT | 2.9 M | `network.connect` with host allowlist + token storage |
| Amazon Q (`amazonwebservices.amazon-q-vscode`) | Apache-2.0 (registry) | 2.2 M | FOSS-licensed but a service dependency; same network capability as Continue |
| Python (`ms-python.python`) | MIT | (registry) | LSP integration DWCode already has for `pyright-langserver`; **Pylance is not included** (proprietary) |
| Go (`golang.Go`) | MIT | (registry) | `gopls` on `PATH`, `process.exec` grant |
| Java (Red Hat) | EPL-2.0 | (registry) | large JVM language server; only after resource limits from blueprint M4 exist |
| rust-analyzer (`rust-lang.rust-analyzer`) | MIT OR Apache-2.0 | (registry) | `rust-analyzer` on `PATH` — DWCode already expects it per README |
| **opencode** (`sst-dev.opencode`) | MIT (upstream `sst/opencode`, 221 K★, verified) | 678 K | the official agent extension: `process.exec` to launch the `opencode` agent + `network.connect`. **Top pick of the AI sweep.** Stage 1 alternative: DWCode ships one agent panel that drives a user-supplied CLI, same pattern as its language-server picker |
| Gemini CLI Companion (`Google.gemini-cli-vscode-ide-companion`) | Apache-2.0 (upstream `google-gemini/gemini-cli`, 108 K★) | 1.59 M | `process.exec` for the `gemini` binary + `network.connect`; bridges the terminal agent into the editor rather than embedding a service SDK |
| Codex / Claude Code history viewer (`HizTam.codex-history-viewer`) | MIT (verified) | 19 K | `workspace.read` only — the lightest host-gated candidate here. Reads local session logs, so DWCode's own session journal makes this a natural **native** feature instead (Stage 1) |
| Varro — OpenCode workbench (`koltyakov.varro`) | MIT (verified) | 52 K | alternative to the official opencode extension: project-aware chat + parallel agent sessions; same `process.exec` + `network.connect` ask |

**AI-agent sweep note (codex / antigravity / gemini / opencode / copilot, checked 2026-10-26 —
200 results across five registry queries, licences re-checked upstream):** every high-download
*agent* extension is proprietary, but **every agent's CLI core is FOSS** — `openai/codex`
Apache-2.0 (131 K★), `sst/opencode` MIT (221 K★), `google-gemini/gemini-cli` Apache-2.0 (108 K★),
Claude Code `NONE`. That is the useful signal: DWCode's agent story should be one panel that
drives a *user-supplied* CLI (`codex`, `opencode`, `gemini`, …) with a `process.exec` grant,
rather than packaging any vendor's editor extension. Third-party OpenCode clients also exist
under other publisher ids (`ZiShuo.opencode` points at the same upstream as `sst-dev.opencode`, and
`haoyangzeng.opencui` "OpenCode Panel", MIT, 113 K) — prefer the official publisher. A third,
`K-Arthur.opencode-harness` (MIT, 11 K when it was listed), now returns **404 from Open VSX**:
delisted, so it cannot be a catalog entry regardless of its licence.

## Excluded — free but not FOSS, or not on Open VSX

| Extension | Why excluded |
| --- | --- |
| GitLens (`eamodio.gitlens`) | GitHub API reports `NOASSERTION` — GitKraken's own freemium/source-available terms, not an OSS license. Free ≠ FOSS. |
| Thunder Client | Proprietary freemium; repository carries no OSS license (GitHub API `NONE`). |
| GitHub Copilot | Proprietary; returns 404 from Open VSX. |
| Pylance, IntelliCode, C++ extension pack extras | Microsoft proprietary components; `ms-vscode.cpptools` returns 404 from Open VSX. |
| Claude Code (`Anthropic.claude-code`) | Proprietary; the registry `license` field is Anthropic's own legal notice, not a license grant — despite 61.5 M downloads. Upstream repo reports `NONE` too. |
| Codex (`openai.chatgpt`, 12.5 M) and Codex Audio (`openai.codex-audio`) | `SEE LICENSE IN LICENSE.md` pointing at proprietary terms; not FOSS. **Distinction that matters:** the *extension* is proprietary while the *CLI core* `openai/codex` is Apache-2.0 — so the capability is reachable through a native `process.exec` integration, never as a package. |
| Gemini Code Assist (`Google.geminicodeassist`, 4.2 M) | `SEE LICENSE IN LICENSE` — service EULA, not a FOSS grant. The FOSS route is the Gemini CLI Companion row in Tier C. |
| Google Antigravity (`Google.google-antigravity`) | `SEE LICENSE IN LICENSE.txt`, no public repo — Google proprietary. |
| Antigravity quota monitors (`jlcodes.antigravity-cockpit` 5.0 M, `n2ns.antigravity-panel` 468 K) | FOSS upstream (MIT / Apache-2.0, both verified) but **no target runtime**: they read Google Antigravity IDE quotas, an IDE DWCode is not. Capability is service-specific, not portable. (`iml1s.antigravity-plus` claims MIT on the registry but its repo is 404 — fails the verify rule regardless.) |
| Code Web Chat / gemini-coder (`robertpiosik.gemini-coder`, 595 K) | GitHub API reports `NONE` — no license file. Stop-and-verify rule applies; high downloads do not substitute for a grant. |
| Copilot bridges (`adguard.tokenguard-copilot`, `statiolake.vscode-copilot-lsp`, `allansantos-dv.copilot-delegate`) | MIT, but each needs GitHub Copilot or the Copilot CLI — proprietary and 404 on Open VSX. FOSS code depending on an out-of-reach product is out of reach as a package. |
| Codex / ChatGPT account switchers (`woozy-masta.codex-switch`, `devhakeem5.codex-hub`, `zixfel.codex-account-swapper`) | Multi-account rotation has no DWCode capability to map to; excluded on usefulness, not licence (`codex-switch` is MIT, verified — the others were not taken further once that failed). |
| SQLite Viewer (`qwtel.sqlite-viewer`, 1.02 M) | Registry `license` reads `LICENSE.md`, but that file is an *"Activate SQLite Viewer PRO"* notice — a paywall, no OSS grant. Free ≠ FOSS, and a plausible-looking license *filename* is not a license. |
| Redis for VS Code (`Redis.redis-for-vscode`, 19 K) | **SSPL-1.0** (`redis/Redis-for-VS-Code`) — source-available but not OSI-approved, so it fails the FOSS rule. The registry pointer hid this; reading the upstream `LICENSE` is what found it. |
| Google Cloud Code (`googlecloudtools.cloudcode`, 430 K) | `SEE LICENSE IN LICENSE` → the *Google Cloud Code Plugin Software License Agreement*. Proprietary service terms, not a grant. |
| DBCode (`dbcode.dbcode`, 875 K) | `SEE LICENSE IN LICENSE` — no SPDX grant; a database service client. |
| Android launcher/emulator helpers (`DiemasMichiels.emulate`, `android-ios-layoutcodegen.*`, `krishna-kudari.android-studio-lite`) | `SEE LICENSE IN` or an empty registry field. Excluded on the gate, not on usefulness — the FOSS Android set is in the sweep above. |
| Devsense PHP / Composer / PHP Profiler (`devsense.*`) | `LicenseRef-LICENSE` — a custom license reference, not an SPDX grant. |
| Live Preview (`ms-vscode.live-server`), Mermaid Chart, Office Viewer, Posit Shiny | Registry license `NONE` or a service EULA. |
| Catppuccin, Tokyo Night theme packages | Marketplace-only (404 on Open VSX); recreation from license-clean sources only. |

"Excluded" applies to *sourcing the package*. A compatible DWCode-native implementation of the
capability (e.g. an AI completion provider with a user-supplied endpoint) is a separate decision
governed by the capability table in [EXTENSIONS.md](EXTENSIONS.md).

**Cleared on 2026-10-26:** `Vue.volar` (registry licence empty) was the last pending row —
upstream `vuejs/language-tools` reads MIT, so it is catalog-eligible and lives in the appendix
now, not above. With it cleared, `npm run catalog:scan` exits **0**: **206 ids checked, 179
catalog-eligible, 0 pending, 20 refused, 7 not on Open VSX**.

## How this drives DWCode's sequence

1. **Now (blueprint M1):** Tier A packs — theme/icon/snippet mapping, enable/disable in
   Settings → EXTENSIONS, license + hash shown before enabling.
2. **Next (Stage 1 built-ins):** Tier B in order of user pain — EditorConfig → Prettier →
   ESLint → spell check (external process, GPL kept at arm's length) → path completion → preview.
   Each lands as a native feature with tests, not as a package.
3. **After the host exists (M2/M3+):** Tier C, capability by capability, with the install prompt
   listing publisher, version, license, package hash, and requested rights. The AI-agent sweep
   points at the first capability to build: **one agent panel driving a user-supplied CLI**
   (`opencode`, `gemini`, `codex` — all FOSS cores) instead of any vendor's editor extension,
   gated behind `process.exec` + `network.connect` and one explicit consent prompt.
4. **Every entry:** pin exact version, record origin + sha256 in the lock record, re-check the
   license field on update — a license change is a reason to re-review, not a silent bump.

## Verification recipe

```sh
# the licence gate for the whole document: re-checks every extension id in this file
npm run catalog:scan                       # exit 1 while anything needs a human look
node tools/openvsx-scan.mjs --keywords kotlin,julia --size 15   # a fresh per-topic sweep
# the full popularity sweep used above (top 80 of the registry)
curl -s 'https://open-vsx.org/api/-/search?query=&size=40&offset=0&orderBy=downloadCount&sortOrder=desc' \
  | jq -r '.extensions[] | [.downloadCount, (.namespace.id + "." + .name), .displayName] | @tsv'
# license + presence for one package
curl -s https://open-vsx.org/api/<namespace>/<name> | jq '{version, license, repository}'
# upstream license when the registry field is empty or a placeholder
curl -s https://api.github.com/repos/<owner>/<repo> | jq '.license.spdx_id'
```

A registry `license` value that is empty, a NUL placeholder (GitLens's `%gitlens.license%`), or a
`NOASSERTION` result means **stop and verify upstream** before the package enters the catalog.

---

## Appendix — every catalog-eligible package (179)

Generated, not written: `node tools/openvsx-scan.mjs --doc --appendix` on 2026-10-26. Re-run it
and replace this table rather than editing rows by hand, so the published list cannot drift from
the gate above. Every row cleared the licence gate — either the registry states an accepted
licence (`accept`) or the upstream `LICENSE` was read and recorded in
[`tools/openvsx-verified.json`](tools/openvsx-verified.json) (`verified`) — and every row is
present on Open VSX right now. Downloads are registry counts at the time of generation; they are
popularity signals, not quality signals.

| Package | Licence | Downloads | Upstream |
| --- | --- | --- | --- |
| Pyrefly - Python Language Tooling (`meta.pyrefly`) | MIT | 92.47 M | [upstream](https://github.com/facebook/pyrefly) |
| Python Debugger (`ms-python.debugpy`) | MIT | 62.65 M | [upstream](https://github.com/microsoft/vscode-python-debugger) |
| Python (`ms-python.python`) | MIT | 61.56 M | [upstream](https://github.com/Microsoft/vscode-python) |
| Language Support for Java(TM) by Red Hat (`redhat.java`) | EPL-2.0 | 46.88 M | [upstream](https://github.com/redhat-developer/vscode-java) |
| Python Environments (`ms-python.vscode-python-envs`) | MIT | 45.22 M | [upstream](https://github.com/microsoft/vscode-python-environments) |
| Ruby LSP (`Shopify.ruby-lsp`) | MIT | 44.52 M | [upstream](https://github.com/Shopify/ruby-lsp) |
| Go (`golang.Go`) | MIT | 42.94 M | [upstream](https://github.com/golang/vscode-go) |
| Project Manager for Java (`vscjava.vscode-java-dependency`) | MIT | 41.49 M | [upstream](https://github.com/Microsoft/vscode-java-dependency) |
| clangd (`llvm-vs-code-extensions.vscode-clangd`) | MIT | 31.55 M | [upstream](https://github.com/clangd/vscode-clangd) |
| Maven for Java (`vscjava.vscode-maven`) | MIT | 28.57 M | [upstream](https://github.com/Microsoft/vscode-maven) |
| Debugger for Java (`vscjava.vscode-java-debug`) | MIT | 27.93 M | [upstream](https://github.com/Microsoft/vscode-java-debug) |
| Gradle for Java (`vscjava.vscode-gradle`) | MIT | 27.9 M | [upstream](https://github.com/microsoft/vscode-gradle) |
| Test Runner for Java (`vscjava.vscode-java-test`) | MIT | 27.84 M | [upstream](https://github.com/Microsoft/vscode-java-test) |
| Extension Pack for Java (`vscjava.vscode-java-pack`) | MIT | 23.77 M | [upstream](https://github.com/Microsoft/vscode-java-pack) |
| Container Tools (`ms-azuretools.vscode-containers`) | MIT | 14.8 M | [upstream](https://github.com/microsoft/vscode-containers) |
| Jupyter (`ms-toolsai.jupyter`) | MIT | 11.08 M | [upstream](https://github.com/Microsoft/vscode-jupyter) |
| Vue (Official) (`Vue.volar`) | MIT | 10.95 M | [upstream](https://github.com/vuejs/language-tools) |
| Prettier - Code formatter (`esbenp.prettier-vscode`) | MIT | 9.65 M | [upstream](https://github.com/prettier/prettier-vscode) |
| YAML (`redhat.vscode-yaml`) | MIT | 8.88 M | [upstream](https://github.com/redhat-developer/vscode-yaml) |
| rust-analyzer (`rust-lang.rust-analyzer`) | MIT OR Apache-2.0 | 7.44 M | [upstream](https://github.com/rust-lang/rust-analyzer) |
| Cline (`saoudrizwan.claude-dev`) | Apache-2.0 | 7.24 M | [upstream](https://github.com/cline/cline) |
| Docker (`ms-azuretools.vscode-docker`) | MIT | 7.13 M | [upstream](https://github.com/microsoft/vscode-docker) |
| Material Icon Theme (`PKief.material-icon-theme`) | MIT | 6.55 M | [upstream](https://github.com/material-extensions/vscode-material-icon-theme) |
| ESLint (`dbaeumer.vscode-eslint`) | MIT | 6.42 M | [upstream](https://github.com/Microsoft/vscode-eslint) |
| Dart (`Dart-Code.Dart-Code`) | MIT | 5.97 M | [upstream](https://github.com/Dart-Code/Dart-Code) |
| Antigravity Cockpit (`jlcodes.antigravity-cockpit`) | MIT | 5.01 M | [upstream](https://github.com/jlcodes99/vscode-antigravity-cockpit) |
| Kilo Code: AI Coding Agent, Copilot, and Autocomplete (`kilocode.kilo-code`) | MIT | 4.62 M | [upstream](https://github.com/Kilo-Org/kilocode) |
| Ruff (`charliermarsh.ruff`) | MIT | 3.91 M | [upstream](https://github.com/astral-sh/ruff-vscode) |
| Rainbow CSV (`mechatroner.rainbow-csv`) | MIT | 3.36 M | [upstream](https://github.com/mechatroner/vscode_rainbow_csv) |
| JavaScript Debugger (`ms-vscode.js-debug`) | MIT | 3.21 M | [upstream](https://github.com/Microsoft/vscode-pwa) |
| Live Server (`ritwickdey.LiveServer`) | MIT | 3.09 M | [upstream](https://github.com/ritwickdey/vscode-live-server) |
| GitLab (`GitLab.gitlab-workflow`) | MIT | 2.9 M | [upstream](https://gitlab.com/gitlab-org/gitlab-vscode-extension) |
| BasedPyright (`detachhead.basedpyright`) | MIT | 2.86 M | [upstream](https://github.com/detachhead/basedpyright) |
| Markdown Preview Enhanced (`shd101wyy.markdown-preview-enhanced`) | NCSA | 2.49 M | [upstream](https://github.com/shd101wyy/vscode-markdown-preview-enhanced) |
| HashiCorp Terraform (`hashicorp.terraform`) | MPL-2.0 | 2.4 M | [upstream](https://github.com/hashicorp/vscode-terraform) |
| EditorConfig (`EditorConfig.EditorConfig`) | MIT | 2.31 M | [upstream](https://github.com/editorconfig/editorconfig-vscode) |
| CMake Tools (`ms-vscode.cmake-tools`) | MIT | 2.25 M | [upstream](https://github.com/microsoft/vscode-cmake-tools) |
| Amazon Q (`amazonwebservices.amazon-q-vscode`) | Apache-2.0 | 2.22 M | [upstream](https://github.com/aws/amazon-q-vscode) |
| Svelte for VS Code (`svelte.svelte-vscode`) | MIT | 2.17 M | [upstream](https://github.com/sveltejs/language-tools) |
| Code Spell Checker (`streetsidesoftware.code-spell-checker`) | GPL-3.0-or-later | 2.17 M | [upstream](https://github.com/streetsidesoftware/vscode-spell-checker) |
| JavaScript Debugger Companion Extension (`ms-vscode.js-debug-companion`) | MIT | 2.16 M | [upstream](https://github.com/microsoft/vscode-js-debug-companion) |
| .NET Install Tool (`ms-dotnettools.vscode-dotnet-runtime`) | MIT | 2.14 M | [upstream](https://github.com/dotnet/vscode-dotnet-runtime) |
| SonarQube for IDE (`SonarSource.sonarlint-vscode`) | LGPL-3.0 | 2.13 M | [upstream](https://github.com/SonarSource/sonarlint-vscode) |
| Table Visualizer for JavaScript Profiles (`ms-vscode.vscode-js-profile-table`) | MIT | 2.1 M | [upstream](https://github.com/microsoft/vscode-js-profile-visualizer) |
| Vim (`vscodevim.vim`) | MIT | 1.98 M | [upstream](https://github.com/VSCodeVim/Vim) |
| markdownlint (`DavidAnson.vscode-markdownlint`) | MIT | 1.84 M | [upstream](https://github.com/DavidAnson/vscode-markdownlint) |
| XML (`redhat.vscode-xml`) | EPL-2.0 | 1.64 M | [upstream](https://github.com/redhat-developer/vscode-xml) |
| Gemini CLI Companion (`Google.gemini-cli-vscode-ide-companion`) | Apache-2.0 | 1.59 M | [upstream](https://github.com/google-gemini/gemini-cli) |
| vscode-icons (`vscode-icons-team.vscode-icons`) | MIT | 1.59 M | [upstream](https://github.com/vscode-icons/vscode-icons) |
| Prisma (`prisma.prisma`) | Apache-2.0 | 1.56 M | [upstream](https://github.com/prisma/language-tools) |
| Black Formatter (`ms-python.black-formatter`) | MIT | 1.53 M | [upstream](https://github.com/microsoft/vscode-black-formatter) |
| MySQL (`cweijan.vscode-mysql-client2`) | MIT | 1.38 M | [upstream](https://github.com/cweijan/vscode-database-client) |
| CodeLLDB (`vadimcn.vscode-lldb`) | MIT | 1.37 M | [upstream](https://github.com/vadimcn/codelldb) |
| Error Lens (`usernamehw.errorlens`) | MIT | 1.36 M | [upstream](https://github.com/usernamehw/vscode-error-lens) |
| Kubernetes (`ms-kubernetes-tools.vscode-kubernetes-tools`) | Apache-2.0 | 1.25 M | [upstream](https://github.com/vscode-kubernetes-tools/vscode-kubernetes-tools) |
| SQLTools (`mtxr.sqltools`) | MIT | 1.13 M | [upstream](https://github.com/mtxr/vscode-sqltools) |
| Code Runner (`formulahendry.code-runner`) | MIT | 1.05 M | [upstream](https://github.com/formulahendry/vscode-code-runner) |
| GraphQL: Syntax Highlighting (`GraphQL.vscode-graphql-syntax`) | MIT | 1.02 M | [upstream](https://github.com/graphql/graphiql) |
| CDT GDB Debug Adapter Extension (`eclipse-cdt.cdt-gdb-vscode`) | EPL-2.0 | 999 K | [upstream](https://github.com/eclipse-cdt-cloud/cdt-gdb-vscode) |
| One Dark Pro (`zhuangtongfa.material-theme`) | MIT | 962 K | [upstream](https://github.com/Binaryify/OneDark-Pro) |
| Jest / Vitest Runner (`firsttris.vscode-jest-runner`) | MIT | 914 K | [upstream](https://github.com/firsttris/vscode-jest-runner) |
| ES7+ React/Redux/React-Native snippets (`dsznajder.es7-react-js-snippets`) | MIT | 908 K | [upstream](https://github.com/dsznajder/vscode-es7-javascript-react-snippets) |
| Markdown Preview Mermaid Support (`bierner.markdown-mermaid`) | MIT | 876 K | [upstream](https://github.com/mjbvz/vscode-markdown-mermaid) |
| Vitest (`vitest.explorer`) | MIT | 869 K | [upstream](https://github.com/vitest-dev/vscode) |
| PHP Debug (`xdebug.php-debug`) | MIT | 796 K | [upstream](https://github.com/xdebug/vscode-php-debug) |
| ShellCheck (`timonwong.shellcheck`) | MIT | 790 K | [upstream](https://github.com/vscode-shellcheck/vscode-shellcheck) |
| HTML CSS Support (`ecmel.vscode-html-css`) | MIT | 790 K | [upstream](https://github.com/ecmel/vscode-html-css) |
| Stylelint (`stylelint.vscode-stylelint`) | MIT | 747 K | [upstream](https://github.com/stylelint/vscode-stylelint) |
| Kotlin (`fwcd.kotlin`) | MIT | 734 K | [upstream](https://github.com/fwcd/vscode-kotlin) |
| Markdown All in One (`yzhang.markdown-all-in-one`) | MIT | 714 K | [upstream](https://github.com/yzhang-gh/vscode-markdown) |
| Even Better TOML (`tamasfe.even-better-toml`) | MIT | 693 K | [upstream](https://github.com/tamasfe/taplo) |
| opencode (`sst-dev.opencode`) | MIT | 678 K | [upstream](https://github.com/sst/opencode) |
| JavaScript and TypeScript Nightly (`ms-vscode.vscode-typescript-next`) | MIT | 677 K | [upstream](https://github.com/microsoft/vscode-typescript-next) |
| Git History (`donjayamanne.githistory`) | MIT | 668 K | [upstream](https://github.com/DonJayamanne/gitHistoryVSCode) |
| Git Blame (`waderyan.gitblame`) | MIT | 614 K | [upstream](https://github.com/Sertion/vscode-gitblame) |
| indent-rainbow (`oderwat.indent-rainbow`) | MIT | 587 K | [upstream](https://github.com/oderwat/vscode-indent-rainbow) |
| GraphQL: Language Feature Support (`GraphQL.vscode-graphql`) | MIT | 584 K | [upstream](https://github.com/graphql/graphiql) |
| Path Intellisense (`christian-kohler.path-intellisense`) | MIT | 572 K | [upstream](https://github.com/ChristianKohler/PathIntellisense) |
| Tombi (`tombi-toml.tombi`) | MIT | 548 K | [upstream](https://github.com/tombi-toml/tombi) |
| SQLTools PostgreSQL/Cockroach Driver (`mtxr.sqltools-driver-pg`) | MIT | 546 K | [upstream](https://github.com/mtxr/vscode-sqltools) |
| Lua (`sumneko.lua`) | MIT | 498 K | [upstream](https://github.com/LuaLS/lua-language-server) |
| MongoDB for VS Code (`mongodb.mongodb-vscode`) | Apache-2.0 | 493 K | [upstream](https://github.com/mongodb-js/vscode) |
| Todo Tree (`gruntfuggly.todo-tree`) | MIT | 491 K | [upstream](https://github.com/Gruntfuggly/todo-tree) |
| C/C++ Debug (`KylinIdeTeam.cppdebug`) | MIT | 482 K | [upstream](https://github.com/quanzhuo/cpp-debug) |
| Antigravity Panel (`n2ns.antigravity-panel`) | Apache-2.0 | 468 K | [upstream](https://github.com/n2ns/antigravity-panel) |
| Dracula Theme Official (`dracula-theme.theme-dracula`) | MIT | 444 K | [upstream](https://github.com/dracula/visual-studio-code) |
| CMake (`twxs.cmake`) | MIT | 431 K | [upstream](https://github.com/twxs/vs.language.cmake) |
| GitHub Theme (`GitHub.github-vscode-theme`) | MIT | 428 K | [upstream](https://github.com/primer/github-vscode-theme) |
| JavaScript (ES6) code snippets (`xabikos.JavaScriptSnippets`) | MIT | 379 K | [upstream](https://github.com/xabikos/vscode-javascript) |
| HTML-validate (`html-validate.vscode-html-validate`) | MIT | 370 K | [upstream](https://gitlab.com/html-validate/vscode-html-validate) |
| autoDocstring - Python Docstring Generator (`njpwerner.autodocstring`) | MIT | 346 K | [upstream](https://github.com/NilsJPWerner/autoDocstring) |
| SQL Server (mssql) (`ms-mssql.mssql`) | MIT | 340 K | [upstream](https://github.com/Microsoft/vscode-mssql) |
| Python Indent (`KevinRose.vsc-python-indent`) | MIT | 338 K | [upstream](https://github.com/kbrose/vsc-python-indent) |
| C/C++ Runner (`franneck94.c-cpp-runner`) | MIT | 333 K | [upstream](https://github.com/franneck94/vscode-c-cpp-runner) |
| Docker DX (`docker.docker`) | Apache-2.0 | 269 K | [upstream](https://github.com/docker/vscode-extension) |
| Julia (`julialang.language-julia`) | MIT | 261 K | [upstream](https://github.com/julia-vscode/julia-vscode) |
| JSON Schema Store Catalog (`remcohaszing.schemastore`) | MIT | 239 K | [upstream](https://github.com/remcohaszing/vscode-schemastore) |
| ElixirLS: Elixir support and debugger (`elixir-lsp.elixir-ls`) | MIT | 234 K | [upstream](https://github.com/elixir-lsp/vscode-elixir-ls) |
| Bash IDE (`mads-hartmann.bash-ide-vscode`) | MIT | 199 K | [upstream](https://github.com/bash-lsp/bash-language-server) |
| CSS Peek (`pranaygp.vscode-css-peek`) | MIT | 180 K | [upstream](https://github.com/pranaygp/vscode-css-peek) |
| SQLite (`alexcvzz.vscode-sqlite`) | Apache-2.0 | 175 K | [upstream](https://github.com/AlexCovizzi/vscode-sqlite) |
| Tools for MicroProfile (`redhat.vscode-microprofile`) | Apache-2.0 | 167 K | [upstream](https://github.com/redhat-developer/vscode-microprofile) |
| Test Adapter Converter (`ms-vscode.test-adapter-converter`) | MIT | 159 K | [upstream](https://github.com/microsoft/vscode-test-adapter-converter) |
| crates (`serayuzgur.crates`) | MIT | 155 K | [upstream](https://github.com/serayuzgur/crates) |
| cmake-format (`cheshirekow.cmake-format`) | GPL-3.0-or-later | 147 K | [upstream](https://github.com/cheshirekow/cmakelang) |
| shell-format (`foxundermoon.shell-format`) | MIT | 143 K | [upstream](https://github.com/foxundermoon/vs-shell-format) |
| Pretty XML (`PrateekMahendrakar.prettyxml`) | MIT | 119 K | [upstream](https://github.com/pmahend1/prettyxml) |
| Google Workspace MCP (`mercurial.google-workspace-mcp`) | MIT | 118 K | [upstream](https://renzojohnson.com/contributions/google-workspace-mcp) |
| OpenCode Panel (`haoyangzeng.opencui`) | MIT | 113 K | [upstream](https://github.com/arthurzengg/opencui) |
| Import Cost (`wix.vscode-import-cost`) | MIT | 110 K | [upstream](https://github.com/wix/import-cost) |
| Harper - Grammar / Spell Checking (`elijah-potter.harper`) | Apache-2.0 | 110 K | [upstream](https://github.com/automattic/harper) |
| C/C++ Compile Run (`danielpinto8zz6.c-cpp-compile-run`) | GPL-3.0 | 109 K | [upstream](https://github.com/danielpinto8zz6/c-cpp-compile-run) |
| Live Sass Compiler (`glenn2223.live-sass`) | MIT | 108 K | [upstream](https://github.com/glenn2223/vscode-live-sass-compiler) |
| Typos spell checker (`tekumara.typos-vscode`) | MIT | 93 K | [upstream](https://github.com/tekumara/typos-lsp) |
| PostgreSQL (`ckolkman.vscode-postgres`) | MIT | 86 K | [upstream](https://github.com/Borvik/vscode-postgres) |
| Bash Debug (`rogalmic.bash-debug`) | MIT | 84 K | [upstream](https://github.com/rogalmic/vscode-bash-debug) |
| Clang-Format (`xaver.clang-format`) | MIT | 81 K | [upstream](https://github.com/xaverh/vscode-clang-format-provider) |
| OpenShift Toolkit (`redhat.vscode-openshift-connector`) | MIT | 74 K | [upstream](https://github.com/redhat-developer/vscode-openshift-tools) |
| Kotlin Language (`mathiasfrohlich.Kotlin`) | Apache-2.0 | 72 K | [upstream](https://github.com/mathiasfrohlich/vscode-kotlin) |
| PHPUnit &amp; Pest Test Explorer (`recca0120.vscode-phpunit`) | MIT | 69 K | [upstream](https://github.com/recca0120/vscode-phpunit) |
| KIDE Test Explorer &amp; Debug (`kide.kide-test-explorer`) | MIT | 66 K | — |
| Total TypeScript (`mattpocock.ts-error-translator`) | MIT | 56 K | [upstream](https://github.com/mattpocock/ts-error-translator) |
| Ruby Sorbet (`sorbet.sorbet-vscode-extension`) | Apache-2.0 | 55 K | [upstream](https://github.com/sorbet/sorbet) |
| Gruvbox Material (`sainnhe.gruvbox-material`) | MIT | 54 K | [upstream](https://github.com/sainnhe/gruvbox-material-vscode) |
| CSS Navigation (`pucelle.vscode-css-navigation`) | MIT | 53 K | [upstream](https://github.com/pucelle/vscode-css-navigation) |
| Varro: OpenCode Workbench (`koltyakov.varro`) | MIT | 53 K | [upstream](https://github.com/koltyakov/varro) |
| Kotlin by JetBrains (`JetBrains.kotlin-server`) | Apache-2.0 | 50 K | [upstream](https://github.com/Kotlin/kotlin-lsp) |
| Vue (`znck.vue`) | MIT | 42 K | [upstream](https://github.com/znck/vue-developer-experience) |
| .NET Meteor (`nromanov.dotnet-meteor`) | MIT | 40 K | [upstream](https://github.com/JaneySprings/DotNet.Meteor) |
| MySQL (`formulahendry.vscode-mysql`) | MIT | 39 K | [upstream](https://github.com/formulahendry/vscode-mysql) |
| Android (`adelphes.android-dev-ext`) | MIT | 39 K | [upstream](https://github.com/adelphes/android-dev-ext) |
| Ruby Solargraph (`castwide.solargraph`) | MIT | 39 K | [upstream](https://github.com/castwide/vscode-solargraph) |
| Cherry Markdown (`cherryMarkdownPublisher.cherry-markdown`) | Apache-2.0 | 38 K | [upstream](https://github.com/Tencent/cherry-markdown) |
| JSON Format (SaltcoreYan) (`saltcoreyan.json-format-sortcore`) | Apache-2.0 | 37 K | [upstream](https://github.com/SaltcoreYan/vscode-json-format) |
| Apollo GraphQL (`apollographql.vscode-apollo`) | MIT | 37 K | [upstream](https://github.com/apollographql/vscode-graphql) |
| YAML Sort (`PascalReitermann93.vscode-yaml-sort`) | MIT | 36 K | [upstream](https://github.com/pascalre/vscode-yaml-sort) |
| OpenCode (`ZiShuo.opencode`) | MIT | 35 K | [upstream](https://github.com/anomalyco/opencode) |
| Git Web Links for VS Code (`reduckted.vscode-gitweblinks`) | MIT | 34 K | [upstream](https://github.com/reduckted/GitWebLinks) |
| JSON Crack (`AykutSarac.jsoncrack-vscode`) | MIT | 33 K | [upstream](https://github.com/AykutSarac/jsoncrack.com) |
| LanguageTool Linter (`davidlday.languagetool-linter`) | Apache-2.0 | 24 K | [upstream](https://github.com/davidlday/vscode-languagetool-linter) |
| Highlight (`fabiospampinato.vscode-highlight`) | MIT | 22 K | [upstream](https://github.com/fabiospampinato/vscode-highlight) |
| Antigravity+ (`iml1s.antigravity-plus`) | MIT | 21 K | [upstream](https://github.com/ImL1s/antigravity-plus) |
| LTeX+ – grammar/spell checking using LanguageTool (`ltex-plus.vscode-ltex-plus`) | MPL-2.0 | 21 K | [upstream](https://github.com/ltex-plus/vscode-ltex-plus) |
| Codex History Viewer - Codex and Claude Code (`HizTam.codex-history-viewer`) | MIT | 19 K | [upstream](https://github.com/hiztam/codex-history-viewer) |
| helm-ls (`helm-ls.helm-ls`) | MIT | 19 K | [upstream](https://github.com/qvalentin/helm-ls-vscode) |
| GraphQL: Inline Operation Execution (`GraphQL.vscode-graphql-execution`) | MIT | 18 K | [upstream](https://github.com/graphql/graphiql) |
| Copilot Delegate (`allansantos-dv.copilot-delegate`) | MIT | 18 K | [upstream](https://github.com/AllanSantos-DV/copilot-delegate) |
| Android ADB WLAN — Wireless Debugging (`HanWang.android-adb-wlan`) | MIT | 15 K | [upstream](https://github.com/deskbtm/android-adb-wlan) |
| KIDE C# (`kide.kide-csharp`) | MIT | 14 K | — |
| Rainbow HTML (`wumbl3.rainbow-html`) | MIT | 14 K | [upstream](https://github.com/realwumbl3/rainbowHTML) |
| Svelte 3 Snippets (`fivethree.vscode-svelte-snippets`) | MIT | 14 K | [upstream](https://github.com/fivethree-team/vscode-svelte-snippets) |
| CMake Test Explorer (`fredericbonnet.cmake-test-adapter`) | BSD-3-Clause | 12 K | [upstream](https://github.com/fredericbonnet/cmake-test-explorer) |
| Codex Switch (`woozy-masta.codex-switch`) | MIT | 11 K | [upstream](https://github.com/WoozyMasta/codex-switch) |
| Android Emulator (`duchuynguyen.android-emulator`) | MIT | 11 K | [upstream](https://github.com/duchuyctlk/vscode-android-emulator) |
| TokenGuard Copilot (`adguard.tokenguard-copilot`) | MIT | 11 K | [upstream](https://github.com/ameshkov/tokenguard-copilot) |
| dotnet-start (`pavel-purma.dotnet-start`) | MIT | 10 K | [upstream](https://github.com/pavel-purma/vscode-dotnet-start) |
| PHP Language Support (`AnessaEah.php-intelephense-language-support`) | MIT | 10 K | [upstream](https://github.com/AnessaEah/vscode-intelephense-extension) |
| Lexical (Elixir Language Server) (`lexical-lsp.lexical`) | Apache-2.0 | 9 K | [upstream](https://github.com/lexical-lsp/vscode-lexical) |
| Bats (Bash Automated Testing System) (`jetmartin.bats`) | MIT | 9 K | [upstream](https://github.com/jetmartin/bats) |
| Julia Formatter (`singularitti.vscode-julia-formatter`) | MIT | 8 K | [upstream](https://github.com/singularitti/vscode-julia-formatter) |
| OpenRA Lua Language Extension (`openra.vscode-openra-lua`) | GPL-3.0 | 7 K | [upstream](https://github.com/OpenRA/vscode-openra-lua) |
| Terraform Plan Colors (`lite2073.tfplan-colors`) | MIT | 6 K | [upstream](https://github.com/litebitus/vscode-tfplan-colors) |
| Kubernetes CRD snippets (`tumido.crd-snippets`) | GPL-3.0-or-later | 6 K | [upstream](https://github.com/tumido/crd-snippets) |
| Ruby Test Explorer (`connorshea.vscode-ruby-test-adapter`) | MIT | 6 K | [upstream](https://github.com/connorshea/vscode-ruby-test-adapter) |
| Android DevKit (`pavi2410.android-devkit`) | MIT | 5 K | [upstream](https://github.com/pavi2410/android-devkit) |
| Expert LSP (`ExpertLSP.expert`) | Apache-2.0 | 5 K | [upstream](https://github.com/expert-lsp/vscode-expert) |
| WoW Lua Language Server (`TradeSkillMaster.wowlua-ls`) | GPL-3.0 | 5 K | [upstream](https://github.com/TradeSkillMaster/wowlua-ls) |
| YAML Doctor (`darioajr.yaml-doctor`) | Apache-2.0 | 4 K | [upstream](https://github.com/darioajr/yaml-doctor-vscode) |
| Qbox Lua - CfxLua &amp; FiveM (`qbox.qbx-lua`) | GPL-3.0-or-later | 4 K | [upstream](https://github.com/Qbox-project/qbx-editor) |
| WebSocket Debug Adapter (`eclipse-cdt.websocket-adapter`) | EPL-2.0 | 4 K | [upstream](https://github.com/eclipse-cdt-cloud/vscode-websocket-adapter) |
| Fix JSON (`oliversturm.fix-json`) | MIT | 4 K | [upstream](https://github.com/oliversturm/vscode-fix-json) |
| Copilot LSP (`statiolake.vscode-copilot-lsp`) | MIT | 4 K | [upstream](https://github.com/statiolake/vscode-copilot-lsp) |
| hex.pm IntelliSense (`benvp.vscode-hex-pm-intellisense`) | MIT | 2 K | [upstream](https://github.com/benlime/vscode-hex-pm-intellisense) |
| Standard Ruby (`testdouble.vscode-standard-ruby`) | MIT | 2 K | [upstream](https://github.com/testdouble/vscode-standard-ruby) |
| gitlab-ci-ls (`alesbrelih.gitlab-ci-ls`) | MIT | 2 K | [upstream](https://github.com/alesbrelih/gitlab-ci-ls) |
| Shellscript LS (`juandiazan.bash-lsp`) | MIT | 1 K | [upstream](https://github.com/juandiazan/shell-language-server) |
| ktfmt - Kotlin Formatter (`omBratteng.ktfmt-kotlin-formatter`) | BSD-3-Clause | 1 K | [upstream](https://github.com/ombratteng/vscode-ktfmt) |
| Android Emulator Viewer (`D0Tech.vs-avd`) | MIT | 788 | [upstream](https://github.com/dzero1/vs-avd) |
| Terraform Graph (`saramorillon.terraform-graph`) | MIT | 744 | [upstream](https://github.com/saramorillon/terraform-graph) |
