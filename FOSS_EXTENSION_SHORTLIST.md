# DWCode FOSS extension shortlist

**Verified:** 2026-10-25 against the live Open VSX registry API (`/api/<ns>/<name>` for license and
presence, `/api/-/search?orderBy=downloadCount&sortOrder=desc` for a full popularity sweep of the
registry's top 80 of 20,163 extensions) and the GitHub license API for packages whose registry
license field was empty, a `SEE LICENSE IN …` pointer, or a placeholder. Extended 2026-10-26 with
an AI-agent sweep — `codex`, `antigravity`, `gemini`, `opencode`, `copilot` (40 results each,
licences re-checked upstream). Re-run these lookups
before each catalog release — licenses and availability change.

**Purpose:** decide which of the most-used free/open-source extensions DWCode should cover, and
*how*: as a declarative pack (blueprint M1), as a built-in integration DWCode implements natively
(EXTENSIONS.md Stage 1), or as a future executable extension behind the capability broker
(blueprint M2/M3+). It narrows and supersedes the raw inventory in
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
| Project Manager/Maven/Gradle/Test for Java (`vscjava.*`) | MIT on the packs; **debug/test report `SEE LICENSE IN` / empty — verify each** | 23–41 M | only the Java language server itself is needed; DWCode consumes `jdtls` output directly |
| clangd (`llvm-vs-code-extensions.vscode-clangd`) | MIT | 31.5 M | DWCode already launches `clangd`; no package needed |
| rust-analyzer | MIT OR Apache-2.0 | 7.4 M | already the documented Rust path |
| Python (`ms-python.python`) | MIT (repo) | 61.6 M | `pyright-langserver` already bundled-adjacent; **Pylance stays excluded** |
| Ruff (`charliermarsh.ruff`) | MIT | 3.9 M | lint/format via one binary; feed diagnostics into the problems panel |
| BasedPyright (`detachhead.basedpyright`) | MIT | 2.9 M | opt-in alternative type checker for Python |
| YAML (`redhat.vscode-yaml`) | MIT | 8.9 M | schema-driven validation → problems panel (schema fetch needs `network.connect`) |
| XML (`redhat.vscode-xml`) | EPL-2.0 | 1.6 M | `lemminx` language server on `PATH` |
| markdownlint | MIT | 1.8 M | (Tier B) rule config reused natively |
| Markdown Preview Enhanced (`shd101wyy.markdown-preview-enhanced`) | **NCSA claimed on registry; repo reports `NOASSERTION` — verify before catalog** | 2.5 M | candidate for DWCode's single built-in preview instead of N preview extensions |
| SonarLint (`SonarSource.sonarlint-vscode`) | **LGPL-3.0** (repo) | 2.1 M | copyleft: run as an external process with its own config, never link statically |
| CMake Tools (`ms-vscode.cmake-tools`) | MIT | 2.3 M | task/run profiles (blueprint task 4) |
| Terraform (`hashicorp.terraform`) | MPL-2.0 | 2.4 M | `terraform-ls` on `PATH`; MPL file-level copyleft is fine for an external server |
| Dart / Flutter (`Dart-Code.*`) | MIT (repo; registry says `SEE LICENSE IN LICENSE`) | 5.1 M / 4.7 M | `dart language-server` on `PATH` |
| CodeLLDB (`vadimcn.vscode-lldb`) | MIT | 1.4 M | DAP adapter candidate once DAP lands |
| JavaScript Debugger (`ms-vscode.js-debug`) | MIT | 3.2 M | DAP adapter for Node; the DAP client is DWCode's own |
| Jupyter (`ms-toolsai.jupyter` + renderers) | MIT | 11.1 M / 10.5 M | notebook UI is a large DWCode feature — decide before committing |
| GitHub Pull Requests / GitHub Actions (`GitHub.*`) | MIT | 6.6 M / 4.5 M | need `network.connect` + auth storage; belongs to the Git milestone, likely built in |
| Container Tools (`ms-azuretools.vscode-containers`) | **registry `SEE LICENSE IN`; upstream repo reports `NOASSERTION` — verify** | 14.8 M | deferred; Docker interaction is `process.exec` territory |
| Intelephense (`bmewburn.vscode-intelephense-client`) | **custom `SEE LICENSE IN` with no public repo — treat as non-FOSS** | 2.2 M | excluded |

## Coverage by area — Rust, C/C++, Git, syntax highlighting, HTML/CSS

Swept per keyword on the registry (top results by downloads with license lookup), so the
"most used" claim is checked rather than assumed.

### Rust

| Item | License | Downloads | DWCode path |
| --- | --- | --- | --- |
| rust-analyzer (`rust-lang.rust-analyzer`) | MIT OR Apache-2.0 | 7.4 M | already the documented Rust path — `rust-analyzer` on `PATH` |
| `rustfmt` / `clippy` via `cargo` | MIT/Apache-2.0 (toolchain) | — | format-on-save and diagnostics through the same external-binary pattern as Prettier/Ruff |
| Even Better TOML (`tamasfe.even-better-toml`) | registry `SEE LICENSE IN LICENSE.md`; upstream repo lookup failed — **verify** | 693 K | `cargo.toml`/`Cargo.lock` parsing; Tombi (`tombi-toml.tombi`, MIT, 548 K) is the FOSS language-server alternative |
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
  not consume TextMate directly — DWCode needs a **tmLanguage → Monarch converter** plus the
  validator extended to check converted grammars. That converter is the missing piece, not the
  packages.
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
under other publisher ids (`ZiShuo.opencode` points at the same upstream as `sst-dev.opencode`;
`haoyangzeng.opencui` "OpenCode Panel", MIT, 113 K, and `K-Arthur.opencode-harness`, MIT, 11 K)
— prefer the official publisher.

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
| Vue - Official (`Vue.volar`) | Registry lists **no license**; upstream `vuejs/language-tools` is MIT — verify at catalog time rather than trusting the registry field. |
| Python Environments (`ms-python.vscode-python-envs`) | Registry lists **no license**; same verify-before-catalog rule as every `NONE` entry. |
| Devsense PHP / Composer / PHP Profiler (`devsense.*`) | `LicenseRef-LICENSE` — a custom license reference, not an SPDX grant. |
| Live Preview (`ms-vscode.live-server`), Mermaid Chart, Office Viewer, Posit Shiny | Registry license `NONE` or a service EULA. |
| Catppuccin, Tokyo Night theme packages | Marketplace-only (404 on Open VSX); recreation from license-clean sources only. |

"Excluded" applies to *sourcing the package*. A compatible DWCode-native implementation of the
capability (e.g. an AI completion provider with a user-supplied endpoint) is a separate decision
governed by the capability table in [EXTENSIONS.md](EXTENSIONS.md).

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
# the full popularity sweep used above (top 80 of the registry)
curl -s 'https://open-vsx.org/api/-/search?query=&size=40&offset=0&orderBy=downloadCount&sortOrder=desc' \
  | jq -r '.extensions[] | [.downloadCount, (.namespace.id + "." + .name), .displayName] | @tsv'
# license + presence for one package
curl -s https://open-vsx.org/api/<namespace>/<name> | jq '{version, license}'
# upstream license when the registry field is empty or a placeholder
curl -s https://api.github.com/repos/<owner>/<repo> | jq '.license.spdx_id'
# a per-topic sweep (how the AI-agent list above was built); repeat per keyword
curl -s 'https://open-vsx.org/api/-/search?query=opencode&size=40&orderBy=downloadCount&sortOrder=desc' \
  | jq -r '.extensions[] | [.downloadCount, (.namespace.id + "." + .name), .displayName] | @tsv'
```

A registry `license` value that is empty, a NUL placeholder (GitLens's `%gitlens.license%`), or a
`NOASSERTION` result means **stop and verify upstream** before the package enters the catalog.
