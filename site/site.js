// DWCode site behaviour: drawer menu, feature sheets, screenshot tour,
// theme explorer, the lazy browser demo and small anime.js effects.
import { animate, stagger, scrambleText, svg } from './vendor/anime.esm.min.js';

const $ = (sel, root = document) => root.querySelector(sel);
const $$ = (sel, root = document) => [...root.querySelectorAll(sel)];
const h = (tag, attrs = {}, ...kids) => {
  const n = document.createElement(tag);
  for (const [k, v] of Object.entries(attrs)) {
    if (k === 'class') n.className = v;
    else if (k === 'html') n.innerHTML = v;
    else if (k.startsWith('on')) n.addEventListener(k.slice(2), v);
    else if (v !== false && v != null) n.setAttribute(k, v === true ? '' : v);
  }
  n.append(...kids.flat().filter((k) => k != null));
  return n;
};
const reduce = matchMedia('(prefers-reduced-motion: reduce)').matches;
const REPO = 'https://github.com/cybercoretech/dwcode';
const src = (path) => `${REPO}/blob/main/${path}`;

// ---------------------------------------------------------------- drawer
// Add future pages here; each group renders as a section of the menu.
// "this page" lists the anchors of the page you are actually on, so the same
// script can run on every page of the site.
const onHome = !location.pathname.endsWith('/repository.html');
const NAV = [
  { label: 'this page', items: (onHome ? [
    { ico: '#', text: 'top', href: '#top' },
    { ico: '01', text: 'features', href: '#features' },
    { ico: '02', text: 'tour', href: '#tour' },
    { ico: '03', text: 'themes', href: '#themes' },
    { ico: '04', text: 'live demo', href: '#demo' },
    { ico: '05', text: 'build from source', href: '#build' },
  ] : [
    { ico: '#', text: 'top', href: '#top' },
    { ico: '01', text: 'the question', href: '#question' },
    { ico: '02', text: 'how it works', href: '#design' },
    { ico: '03', text: 'license gate', href: '#gate' },
    { ico: '04', text: 'excluded list', href: '#excluded' },
    { ico: '05', text: 'submit a pack', href: '#submit' },
  ]) },
  { label: 'pages', items: [
    { ico: '~', text: 'home', href: '/', current: onHome },
    { ico: '~', text: 'extension repository', href: '/repository.html', current: !onHome },
  ] },
  { label: 'project', items: [
    { ico: '↗', text: 'github repository', href: REPO, ext: true, note: 'new tab' },
    { ico: '↗', text: 'issues', href: `${REPO}/issues`, ext: true, note: 'new tab' },
    { ico: '↗', text: 'cybercore tech', href: 'https://cybercoretech.net/', ext: true, note: 'new tab' },
  ] },
];
const drawer = $('#drawer'), scrim = $('.scrim'), burger = $('.burger');
$('#drawer-body').append(...NAV.map((g) => h('div', { class: 'drawer-group' }, h('p', {}, `// ${g.label}`),
  ...g.items.map((it) => h('a', { href: it.href, 'aria-current': it.current ? 'true' : null, ...(it.ext ? { target: '_blank', rel: 'noopener' } : {}) },
    h('span', { class: 'ico', 'aria-hidden': 'true' }, it.ico), it.text, it.note ? h('small', {}, it.note) : null)))));
function setDrawer(open) {
  if (open) { drawer.hidden = false; scrim.hidden = false; requestAnimationFrame(() => drawer.classList.add('open')); }
  else { drawer.classList.remove('open'); scrim.hidden = true; setTimeout(() => { if (!drawer.classList.contains('open')) drawer.hidden = true; }, 300); }
  burger.setAttribute('aria-expanded', String(open));
  burger.setAttribute('aria-label', open ? 'Close menu' : 'Open menu');
  if (open) $('a', drawer)?.focus(); else burger.focus({ preventScroll: true });
}
burger.addEventListener('click', () => setDrawer(burger.getAttribute('aria-expanded') !== 'true'));
$('.drawer-close').addEventListener('click', () => setDrawer(false));
scrim.addEventListener('click', () => setDrawer(false));
drawer.addEventListener('click', (e) => { if (e.target.closest('a[href^="#"]')) setDrawer(false); });
addEventListener('keydown', (e) => { if (e.key === 'Escape' && drawer.classList.contains('open')) setDrawer(false); });

// ---------------------------------------------------------------- features
// Facts are taken from the source (src/workspacebackend.cpp, Main.qml) and the test suites.
const FEATURES = [
  {
    id: 'workspaces', glyph: '⌂', title: 'Real workspaces', status: 'working',
    blurb: 'Open any folder, browse it in the explorer and edit in tabs. Paths stay inside the workspace.',
    summary: 'DWCode opens a real folder from disk (Ctrl+O, the Open folder button, or --workspace on the command line), lists it in the explorer, and opens text files into shared documents, so every tab and split pane sees the same buffer.',
    how: ['Every path is canonicalised and must stay under the workspace root; symlinks that point outside are refused.', 'Files must be valid UTF-8 without NUL bytes; binaries are rejected instead of being mangled.', 'Build and dependency folders are skipped when listing, so huge trees stay responsive.'],
    stats: [['8 MiB', 'largest file opened'], ['3,000', 'explorer entries max'], ['10', 'folder depth scanned'], ['8', 'folders skipped (.git, node_modules…)']],
    tests: ['BackendTest::files', 'UiTest::saveFromEditorKeyboardAndButton'],
    next: ['File watching for external changes', 'A New File command'],
    links: [['source: workspacebackend.cpp', src('src/workspacebackend.cpp')], ['tests: backend_test.cpp', src('tests/backend_test.cpp')]],
  },
  {
    id: 'saving', glyph: '⤓', title: 'Safe saving', status: 'working',
    blurb: 'Atomic writes on Ctrl+S. If the file changed on disk behind you, the save is blocked.',
    summary: 'Saves go through Qt\'s QSaveFile: the new content is written to a temporary file and swapped in only when complete, so a crash mid-save never leaves half a file.',
    how: ['Before writing, DWCode hashes what is on disk and compares it with what it loaded. If someone else changed the file, the save is refused and your edits stay in the editor.', 'Save As writes only inside the workspace and never overwrites an existing file.', 'Closing the window or switching folders with unsaved changes asks first.'],
    stats: [['SHA-256', 'disk-change check'], ['atomic', 'QSaveFile commit'], ['Ctrl+S', 'save'], ['Ctrl+Shift+S', 'save as']],
    tests: ['BackendTest::files', 'UiTest::saveFromEditorKeyboardAndButton'],
    next: ['Crash recovery for unsaved buffers'],
    links: [['source: writeDocument()', src('src/workspacebackend.cpp')], ['tests: ui_test.cpp', src('tests/ui_test.cpp')]],
  },
  {
    id: 'lsp', glyph: 'λ', title: 'Language servers', status: 'working',
    blurb: 'Diagnostics, completion, hover and go-to-definition over LSP.',
    summary: 'Connect LSP starts a language server for the active file and speaks the Language Server Protocol to it over stdio. Errors appear as underlines and in the Problems panel; completion, hover and definition come from the server.',
    how: ['TypeScript and JavaScript use the bundled typescript-language-server with TypeScript 5.9.3.', 'C/C++, Rust and Python use clangd, rust-analyzer and pyright-langserver if they are on your PATH.', 'Requests time out instead of hanging the editor; a malformed server message disconnects cleanly.'],
    stats: [['4', 'server families'], ['4', 'features: diag · complete · hover · definition'], ['15 s', 'initialise timeout'], ['10 s', 'request timeout']],
    tests: ['BackendTest::languageServer', 'UiTest::languageServerThroughEditor'],
    next: ['Discover and install servers from inside DWCode', 'End-to-end tests for clangd, rust-analyzer and pyright'],
    links: [['source: LSP client', src('src/workspacebackend.cpp')], ['editor glue: main.js', src('src/main.js')]],
  },
  {
    id: 'terminal', glyph: '›_', title: 'Workspace terminal', status: 'partial',
    blurb: 'Run commands in the project, stream output, send input, stop them. A command runner for now.',
    summary: 'The Terminal tab runs a command with bash in the workspace folder and streams stdout and stderr into the panel. While it runs, the input line sends text to the process; Stop ends it and everything it started.',
    how: ['Commands run as bash --noprofile --norc -c with TERM=dumb and NO_COLOR=1, so output stays plain text.', 'Stop sends SIGTERM to the whole process group, then SIGKILL if it hasn\'t exited after 600 ms.', 'It is not a PTY yet, so full-screen programs such as vim or htop don\'t work.'],
    stats: [['bash', 'shell'], ['200k', 'characters of scrollback'], ['600 ms', 'grace before SIGKILL'], ['1', 'command at a time']],
    tests: ['BackendTest::terminal', 'UiTest::terminalRunsSendsInputAndStops'],
    next: ['A real PTY terminal with persistent shell state'],
    links: [['source: runCommand()', src('src/workspacebackend.cpp')], ['panel: Main.qml', src('src/qml/Main.qml')]],
  },
  {
    id: 'themes', glyph: '◐', title: '84 Cybercore themes', status: 'working',
    blurb: 'Every Cybercore palette plus three DWCode originals, applied to shell and editor together.',
    summary: 'The theme chooser in the top bar lists the canonical Cybercore palettes. One choice recolours the Qt shell and Monaco together and is remembered for next time.',
    how: ['Palettes are imported from the Cybercore theme schema by tools/sync-cybercore-themes.py, keeping the MIT notice.', 'Each palette maps surfaces, text, accents and syntax colours into both the Qt shell and Monaco.', 'Browse them all in the theme explorer below.'],
    stats: [['84', 'palettes'], ['8', 'families'], ['2', 'surfaces recoloured: shell + editor'], ['QSettings', 'remembered across restarts']],
    tests: ['BackendTest::themes', 'UiTest::themeDropdownOpensScrollsAndApplies', 'UiTest::everyThemeReachesTheEditor', 'UiTest::themeSurvivesRestart'],
    next: ['Theme packs loaded at runtime'],
    links: [['data: themes.json', src('src/data/themes.json')], ['importer', src('tools/sync-cybercore-themes.py')], ['explore themes ↓', '#themes']],
  },
  {
    id: 'keyboard', glyph: '⫴', title: 'Split view, keyboard first', status: 'working',
    blurb: 'Both panes share one document. Command palette on Ctrl+P, optional Helix-style modal mode.',
    summary: 'Split view puts two editors side by side on the same document, so a change in one appears in the other at once. Everything has a keyboard path, and an optional modal mode borrows Helix-style motions.',
    keys: [['Ctrl+O', 'open folder'], ['Ctrl+S', 'save'], ['Ctrl+P', 'command palette'], ['Ctrl+B', 'toggle explorer'], ['Ctrl+J', 'toggle terminal'], ['Ctrl+\\', 'split editor'], ['Ctrl+Shift+M', 'toggle modal mode'], ['Esc / i', 'normal / insert'], ['h j k l', 'move (normal mode)'], ['x', 'delete right (normal mode)']],
    stats: [['2', 'panes on one document'], ['8', 'commands in the palette'], ['10', 'shortcuts'], ['0 ms', 'sync between panes (shared model)']],
    tests: ['UiTest::splitViewSharesThemeAndText'],
    next: ['User-defined keymaps'],
    links: [['shortcuts: Main.qml', src('src/qml/Main.qml')], ['try modal mode in the demo ↓', '#demo']],
  },
];
const grid = $('#feature-grid');
if (grid) for (const f of FEATURES) {
  grid.append(h('button', { class: 'card', type: 'button', 'aria-haspopup': 'dialog', onclick: () => openSheet(f) },
    h('div', { class: 'card-top' }, h('span', { class: 'glyph', 'aria-hidden': 'true' }, f.glyph), h('h3', {}, f.title)),
    h('p', {}, f.blurb),
    h('div', { class: 'card-foot' }, h('span', { class: `status ${f.status === 'partial' ? 'partial' : ''}` }, f.status === 'partial' ? 'works · limits' : 'working'), h('span', { class: 'open' }, 'details →'))));
}
const sheet = $('#feature-sheet');
function openSheet(f) {
  $('#sheet-file').textContent = `features/${f.id}.md`;
  const body = $('#sheet-body');
  body.replaceChildren(...[
    h('h3', { id: 'sheet-title' }, h('span', { class: 'glyph', 'aria-hidden': 'true' }, f.glyph), f.title),
    h('p', { class: 'sum' }, f.summary),
    h('div', { class: 'stats' }, f.stats.map(([v, l]) => h('div', { class: 'stat' }, h('b', {}, v), h('span', {}, l)))),
    f.how ? [h('h4', {}, '// how it works'), h('ul', {}, f.how.map((t) => h('li', {}, t)))] : null,
    f.keys ? [h('h4', {}, '// keys'), h('div', { class: 'keys' }, f.keys.flatMap(([k, d]) => [h('kbd', {}, k), h('span', {}, d)]))] : null,
    h('h4', {}, '// covered by tests'), h('div', { class: 'tests' }, f.tests.map((t) => h('div', { class: 'test' }, h('code', {}, t)))),
    h('h4', {}, '// next'), h('ul', {}, f.next.map((t) => h('li', {}, t))),
    h('h4', {}, '// links'), h('div', { class: 'links' }, f.links.map(([t, href]) => h('a', { href, ...(href.startsWith('#') ? { onclick: () => sheet.close() } : { target: '_blank', rel: 'noopener' }) }, t))),
    h('p', { class: 'telemetry', html: '<b>telemetry: none.</b> DWCode sends nothing anywhere; the editor is served from a loopback-only listener on your machine.' }),
  ].flat(2).filter(Boolean));
  sheet.showModal();
  body.scrollTop = 0;
  if (!reduce) animate($$('.stat, .test', body), { opacity: [0, 1], translateY: [6, 0], delay: stagger(30), duration: 380, ease: 'outQuad' });
}
$('.sheet-close')?.addEventListener('click', () => sheet.close());
sheet?.addEventListener('click', (e) => { if (e.target === sheet) sheet.close(); });

// ---------------------------------------------------------------- tour
const SHOTS = [
  ['completion', 'Completion', 'Member completion from the TypeScript language server after typing routes.'],
  ['terminal', 'Terminal', 'The workspace terminal running ls, grep and wc in the project, with the next command ready.'],
  ['split', 'Split view', 'Split view: both panes edit the same document.'],
  ['themes', 'Theme chooser', 'The Cybercore theme chooser, open over the editor.'],
  ['hero', 'Diagnostics', 'Hover, an error underline and the problems panel, all from the language server.'],
];
const tourStage = $('#tour-stage'), tourTabs = $('#tour-tabs');
// A fresh <img> per switch: changing srcset on a live image left HiDPI screens blank.
function shotImg([name, , alt]) {
  return h('img', { src: `screenshots/${name}.webp`, srcset: `screenshots/${name}.webp 1440w, screenshots/${name}@2x.webp 2880w`,
    sizes: '(max-width: 1180px) 100vw, 1148px', width: 1440, height: 900, alt, decoding: 'async' });
}
async function showShot(i) {
  $$('[role="tab"]', tourTabs).forEach((t, j) => t.setAttribute('aria-selected', String(i === j)));
  const next = shotImg(SHOTS[i]);
  $('#tour-cap').textContent = `// ${SHOTS[i][2]}`;
  try { await next.decode(); } catch { /* show it anyway */ }
  const old = $$('img', tourStage);
  tourStage.append(next);
  old.forEach((o) => { o.classList.add('leaving'); setTimeout(() => o.remove(), 320); });
}
if (tourTabs) {
  SHOTS.forEach((s, i) => tourTabs.append(h('button', { role: 'tab', 'aria-selected': 'false', 'aria-controls': 'tour-panel', onclick: () => showShot(i) }, s[1])));
  showShot(0);
  // warm the rest so switching is instant
  addEventListener('load', () => SHOTS.slice(1).forEach((s) => { const i = shotImg(s); i.decode?.().catch(() => {}); }));
}

// Arrow keys between tabs in any tablist
for (const list of $$('[role="tablist"]')) list.addEventListener('keydown', (e) => {
  if (!['ArrowLeft', 'ArrowRight'].includes(e.key)) return;
  const items = $$('[role="tab"]', list), i = items.indexOf(document.activeElement);
  if (i < 0) return;
  const n = items[(i + (e.key === 'ArrowRight' ? 1 : items.length - 1)) % items.length];
  n.focus(); n.click(); e.preventDefault();
});

// ---------------------------------------------------------------- themes
const FAMILY_LABEL = { cyberpunk: 'cyberpunk', cyberdyne: 'cyberdyne', synthwave: 'synthwave', neosynth: 'neosynth', dystopian: 'dystopian', default: 'classics', 'omarchy-live': 'omarchy-live', DWCode: 'dwcode' };
const FAMILY_ORDER = ['DWCode', 'cyberpunk', 'cyberdyne', 'synthwave', 'neosynth', 'dystopian', 'omarchy-live', 'default'];
// relay.ts excerpt, pre-tokenised: [syntax role, text]
const SAMPLE = [
  [['keyword', 'import'], ['punct', ' { '], ['keyword', 'type'], ['type', ' Signal'], ['punct', ', '], ['function', 'isStrong'], ['punct', ' } '], ['keyword', 'from'], ['string', ' "./signal"'], ['punct', ';']],
  [],
  [['comment', '/** Groups strong signals by channel, newest first. */']],
  [['keyword', 'export function '], ['function', 'route'], ['punct', '('], ['variable', 'signals'], ['punct', ': '], ['type', 'Signal'], ['punct', '[]) {']],
  [['punct', '  '], ['keyword', 'const '], ['variable', 'routes'], ['punct', ' = '], ['keyword', 'new '], ['type', 'Map'], ['punct', '<'], ['type', 'string'], ['punct', ', '], ['type', 'Signal'], ['punct', '[]>();']],
  [['punct', '  '], ['keyword', 'for '], ['punct', '('], ['keyword', 'const '], ['variable', 'signal'], ['keyword', ' of '], ['variable', 'signals'], ['punct', '.'], ['function', 'filter'], ['punct', '('], ['function', 'isStrong'], ['punct', ')) {']],
  [['punct', '    '], ['variable', 'routes'], ['punct', '.'], ['function', 'set'], ['punct', '('], ['variable', 'signal'], ['punct', '.channel, ['], ['variable', 'signal'], ['punct', ']);']],
  [['punct', '  }']],
  [['punct', '  '], ['keyword', 'return '], ['variable', 'routes'], ['punct', ';']],
  [['punct', '}']],
  [],
  [['keyword', 'const '], ['constant', 'THRESHOLD'], ['punct', ' = '], ['number', '0.75'], ['punct', ';  '], ['comment', '// strong enough to route']],
  [['function', 'route'], ['punct', '([{ node: '], ['string', '"darkbox"'], ['punct', ', strength: '], ['number', '0.92'], ['punct', ' }]);']],
];
let themes = [], selectedTheme = 'cyberpunk-chrome';
const tree = $('#theme-tree'), preview = $('#theme-preview');

function renderPreview(t) {
  const s = t.surfaces, tx = t.text, sx = t.syntax;
  const color = (role) => role === 'punct' ? tx.primary : (sx[role] || tx.primary);
  preview.style.background = s.editor;
  preview.replaceChildren(
    h('div', { class: 'pv-title', style: `background:${s.shell};color:${tx.muted};border-color:${s.border}` },
      h('span', {}, h('b', { style: `color:${tx.primary}` }, t.name), `  ·  ${FAMILY_LABEL[t.family] || t.family} · ${t.appearance}`), h('span', {}, `${t.id}.json`)),
    h('div', { class: 'pv-tabs', style: `background:${s.shell};border-color:${s.border}` },
      h('span', { class: 'pv-tab', style: `background:${s.editor};color:${tx.primary};border-color:${s.border};box-shadow:inset 0 2px 0 ${t.accents.primary}` }, 'relay.ts'),
      h('span', { class: 'pv-tab', style: `color:${tx.muted};border-color:transparent` }, 'signal.ts')),
    h('pre', { class: 'pv-code', style: `color:${tx.primary}` }, SAMPLE.map((line, i) =>
      h('div', { class: `row${i === 4 ? ' cur' : ''}`, style: i === 4 ? `background:${s.shell};color:${t.accents.primary}` : '' },
        h('span', { class: 'num', style: `color:${i === 4 ? t.accents.primary : tx.muted}` }, String(i + 1)),
        h('span', {}, line.map(([role, text]) => h('span', { style: `color:${color(role)}${role === 'comment' ? ';font-style:italic' : ''}` }, text)))))),
    h('div', { class: 'pv-meta' },
      [['editor', s.editor], ['primary', t.accents.primary], ['secondary', t.accents.secondary], ['keyword', sx.keyword], ['string', sx.string], ['function', sx.function]]
        .map(([l, c]) => h('span', { class: 'chip', title: c }, h('i', { style: `background:${c}` }), `${l} ${c.toLowerCase()}`)),
      h('button', { class: 'btn btn-primary btn-sm', type: 'button', onclick: () => { startDemo(); applyTheme(t.id); $('#demo').scrollIntoView({ block: 'start' }); } }, '▶ load in demo')),
  );
}
function selectTheme(id, focus = false) {
  selectedTheme = id;
  const t = themes.find((x) => x.id === id);
  if (!t) return;
  $$('.leaf', tree).forEach((l) => l.setAttribute('aria-selected', String(l.dataset.id === id)));
  const leaf = $(`.leaf[data-id="${CSS.escape(id)}"]`, tree);
  const list = leaf?.closest('ul');
  if (list?.hidden) toggleFolder(list.previousElementSibling, true);
  if (focus) leaf?.focus();
  renderPreview(t);
}
function toggleFolder(btn, open) {
  const list = btn.nextElementSibling;
  const want = open ?? list.hidden;
  list.hidden = !want;
  btn.setAttribute('aria-expanded', String(want));
}
function buildTree() {
  const fams = FAMILY_ORDER.filter((f) => themes.some((t) => t.family === f))
    .concat([...new Set(themes.map((t) => t.family))].filter((f) => !FAMILY_ORDER.includes(f)));
  $('#theme-total').textContent = themes.length;
  $('#family-total').textContent = fams.length;
  tree.replaceChildren(h('div', { class: 'tree-title' }, `THEMES · ${themes.length}`));
  for (const fam of fams) {
    const list = themes.filter((t) => t.family === fam);
    const btn = h('button', { type: 'button', 'aria-expanded': 'false', onclick: () => toggleFolder(btn) },
      h('span', { class: 'chev', 'aria-hidden': 'true' }, '›'), h('span', { class: 'fico', 'aria-hidden': 'true' }, '▸'), FAMILY_LABEL[fam] || fam, h('small', {}, list.length));
    btn.querySelector('.fico').textContent = '▤';
    const ul = h('ul', { role: 'group', hidden: true }, list.map((t) => h('li', { role: 'none' },
      h('button', { class: 'leaf', type: 'button', role: 'treeitem', 'data-id': t.id, 'aria-selected': 'false', onclick: () => selectTheme(t.id) },
        h('span', { class: 'sw', style: `background:linear-gradient(135deg,${t.surfaces.editor} 0 45%,${t.accents.primary} 45% 70%,${t.accents.secondary} 70%)` }),
        t.name.replace(/^(Cyberpunk|Cyberdyne|Synthwave|Neosynth|Dystopian|DWCode)\s+/, ''), h('span', { class: 'ext' }, '.json')))));
    tree.append(h('div', { class: 'folder' }, btn, ul));
  }
  // keyboard: up/down through visible leaves
  tree.addEventListener('keydown', (e) => {
    if (!['ArrowDown', 'ArrowUp'].includes(e.key)) return;
    const items = $$('.folder > button, .leaf', tree).filter((n) => n.offsetParent);
    const i = items.indexOf(document.activeElement);
    if (i < 0) return;
    items[Math.max(0, Math.min(items.length - 1, i + (e.key === 'ArrowDown' ? 1 : -1)))].focus();
    e.preventDefault();
  });
  toggleFolder($('.folder > button', tree), true);
  selectTheme(selectedTheme);
}

// ---------------------------------------------------------------- demo
const demo = $('.demo'), stage = $('#demo-stage'), select = $('#demo-theme');
let frame = null, pendingTheme = null;
const demoWindow = () => (frame?.contentWindow?.dwcodeEditor ? frame.contentWindow : null);
function startDemo() {
  if (frame) return;
  frame = h('iframe', { src: 'demo/', title: 'DWCode editor demo' });
  frame.addEventListener('load', () => {
    const ready = () => {
      const w = demoWindow();
      if (!w) return setTimeout(ready, 60);
      demo.classList.add('live');
      applyTheme(pendingTheme || select.value || selectedTheme);
    };
    ready();
  });
  $('#demo-start')?.replaceWith(frame);
}
$('#demo-start')?.addEventListener('click', startDemo);
function applyTheme(id) {
  select.value = id;
  const w = demoWindow();
  if (!w) { pendingTheme = id; return; }
  pendingTheme = null;
  w.dwcodeSetTheme(id);
  const t = themes.find((x) => x.id === id);
  if (t) stage.style.background = t.surfaces.editor;
}
select?.addEventListener('change', () => { applyTheme(select.value); selectTheme(select.value); });
$$('.demo-files [role="tab"]').forEach((tab, _, all) => tab.addEventListener('click', () => {
  const w = demoWindow();
  if (!w) return;
  all.forEach((t) => t.setAttribute('aria-selected', String(t === tab)));
  w.dwcodeOpenSample(tab.dataset.file);
  w.dwcodeEditor.focus();
}));
const modal = $('#demo-modal');
modal?.addEventListener('click', () => {
  const w = demoWindow();
  if (!w) return;
  const on = modal.getAttribute('aria-pressed') !== 'true';
  modal.setAttribute('aria-pressed', String(on));
  modal.textContent = `modal: ${on ? 'on' : 'off'}`;
  w.dwcodeSetMode(on);
  w.dwcodeEditor.focus();
});

if (tree && select) fetch('data/themes.json').then((r) => r.json()).then((list) => {
  themes = list;
  for (const t of list) select.append(new Option(t.name, t.id));
  select.value = selectedTheme;
  buildTree();
}).catch(() => { if (tree) tree.textContent = 'Theme list unavailable right now.'; });

// ---------------------------------------------------------------- logo
// The Bracket mark (Monaspace Krypton, outlined): wordmark in the bar, full
// lockup in the hero. Brackets draw in, letters type, the cursor blinks.
function placeLogo(file, holder, before) {
  if (!holder) return Promise.resolve();
  return fetch(`assets/${file}`).then((r) => r.ok ? r.text() : Promise.reject(r.status)).then((markup) => {
    before?.remove();
    holder.insertAdjacentHTML('afterbegin', markup);
    const logo = $('svg', holder);
    logo.removeAttribute('width'); logo.removeAttribute('height');
    logo.setAttribute('aria-hidden', 'true');
    if (reduce) return;
    animate(svg.createDrawable($$('.br', logo)), { draw: ['0 0', '0 1'], duration: 700, ease: 'inOutQuad' });
    animate($$('.br-l', logo), { translateX: [14, 0], duration: 700, ease: 'outExpo' });
    animate($$('.br-r', logo), { translateX: [-14, 0], duration: 700, ease: 'outExpo' });
    animate($$('[class^="ch ch-"]', logo), { opacity: [0, 1], duration: 1, delay: stagger(85, { start: 380 }) });
  }).catch(() => { /* keep the fallback */ });
}
placeLogo('dwcode-wordmark.svg', $('.wordmark'), $('.wordmark .wm-fallback'));
placeLogo('dwcode-lockup.svg', $('.hero-logo'));

// ---------------------------------------------------------------- copy
$$('[data-copy]').forEach((btn) => btn.addEventListener('click', async () => {
  const text = $(`#${btn.dataset.copy}`).innerText.split('\n').map((l) => l.replace(/^\s*\d+\s?/, '')).join('\n').trim();
  try { await navigator.clipboard.writeText(text); btn.textContent = 'copied'; }
  catch { btn.textContent = 'select & copy'; }
  setTimeout(() => { btn.textContent = 'copy'; }, 1600);
}));

// ---------------------------------------------------------------- motion
if (!reduce) {
  // Headings resolve from scrambled glyphs the first time they scroll into view.
  const scramble = (el) => {
    const parts = el.children.length ? [...el.childNodes].map((n) => {
      if (n.nodeType === 3) { const s = h('span', {}, n.textContent); n.replaceWith(s); return s; }
      return n;
    }) : [el];
    animate(parts, { innerHTML: scrambleText({ chars: '01<>/{}[]=;#$_', cursor: '▌', revealRate: 90, settleDuration: 220 }) });
  };
  const seen = new IntersectionObserver((entries) => entries.forEach((e) => {
    if (!e.isIntersecting) return;
    seen.unobserve(e.target);
    if (e.target.matches('[data-scramble]')) scramble(e.target);
    if (e.target.id === 'feature-grid') animate($$('.card', e.target), { opacity: [0, 1], translateY: [14, 0], delay: stagger(70), duration: 600, ease: 'outExpo' });
  }), { threshold: .3 });
  $$('[data-scramble]').forEach((el) => seen.observe(el));
  if (grid) seen.observe(grid);
  // Terminal lines print in sequence.
  animate($$('#term .tl'), { opacity: [0, 1], translateX: [-6, 0], delay: stagger(260, { start: 300 }), duration: 300, ease: 'outQuad' });
  // Subtle tilt on feature cards.
  for (const card of $$('.card')) {
    card.addEventListener('pointermove', (e) => {
      const r = card.getBoundingClientRect(), x = (e.clientX - r.left) / r.width - .5, y = (e.clientY - r.top) / r.height - .5;
      card.style.transform = `perspective(700px) rotateX(${(-y * 4).toFixed(2)}deg) rotateY(${(x * 5).toFixed(2)}deg) translateY(-2px)`;
    });
    card.addEventListener('pointerleave', () => { card.style.transform = ''; });
  }
}
