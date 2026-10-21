// DWCode site behaviour: screenshot tabs, theme gallery, the lazy browser
// demo (the desktop app's Monaco bundle in an iframe) and copy buttons.

const $ = (sel, root = document) => root.querySelector(sel);
const $$ = (sel, root = document) => [...root.querySelectorAll(sel)];

function swapImage(img, base, alt) {
  img.classList.add('swap');
  const next = new Image();
  next.src = `screenshots/${base}.webp`;
  next.decode().catch(() => {}).finally(() => {
    img.src = next.src;
    if (img.hasAttribute('srcset')) img.srcset = `screenshots/${base}.webp 1440w, screenshots/${base}@2x.webp 2880w`;
    img.alt = alt;
    img.classList.remove('swap');
  });
}

// Arrow keys move between options in a tablist/radiogroup.
function arrowNav(group, selector) {
  group.addEventListener('keydown', (e) => {
    if (!['ArrowLeft', 'ArrowRight'].includes(e.key)) return;
    const items = $$(selector, group);
    const i = items.indexOf(document.activeElement);
    if (i < 0) return;
    const next = items[(i + (e.key === 'ArrowRight' ? 1 : items.length - 1)) % items.length];
    next.focus();
    next.click();
    e.preventDefault();
  });
}

// Tour tabs
const tourTabs = $$('.tabs [role="tab"]');
tourTabs.forEach((tab) => tab.addEventListener('click', () => {
  tourTabs.forEach((t) => t.setAttribute('aria-selected', String(t === tab)));
  swapImage($('#tour-img'), tab.dataset.shot, tab.dataset.cap);
  $('#tour-cap').textContent = tab.dataset.cap;
}));
arrowNav($('.tabs'), '[role="tab"]');

// Theme screenshot picker
const themePicks = $$('.theme-picks [role="radio"]');
themePicks.forEach((pick) => pick.addEventListener('click', () => {
  themePicks.forEach((p) => p.setAttribute('aria-checked', String(p === pick)));
  swapImage($('#theme-img'), `theme-${pick.dataset.theme}`, `DWCode in the ${pick.textContent} theme.`);
}));
arrowNav($('.theme-picks'), '[role="radio"]');

// Live demo
const demo = $('.demo');
const stage = $('#demo-stage');
const select = $('#demo-theme');
let frame = null;
let pendingTheme = null;

function demoWindow() {
  return frame?.contentWindow?.dwcodeEditor ? frame.contentWindow : null;
}

function startDemo() {
  if (frame) return;
  frame = document.createElement('iframe');
  frame.src = 'demo/index.html';
  frame.title = 'DWCode editor demo';
  frame.addEventListener('load', () => {
    const ready = () => {
      const w = demoWindow();
      if (!w) return setTimeout(ready, 60);
      demo.classList.add('live');
      if (pendingTheme) applyTheme(pendingTheme);
      else if (select.value) applyTheme(select.value);
    };
    ready();
  });
  $('#demo-start').replaceWith(frame);
}
$('#demo-start').addEventListener('click', startDemo);

function applyTheme(id) {
  select.value = id;
  $$('.swatch').forEach((s) => s.setAttribute('aria-current', String(s.dataset.id === id)));
  const w = demoWindow();
  if (!w) { pendingTheme = id; return; }
  pendingTheme = null;
  w.dwcodeSetTheme(id);
  const theme = themes.find((t) => t.id === id);
  if (theme) stage.style.background = theme.surfaces.editor;
}
select.addEventListener('change', () => applyTheme(select.value));

$$('.demo-files [role="tab"]').forEach((tab, _, all) => tab.addEventListener('click', () => {
  const w = demoWindow();
  if (!w) return;
  all.forEach((t) => t.setAttribute('aria-selected', String(t === tab)));
  w.dwcodeOpenSample(tab.dataset.file);
  w.dwcodeEditor.focus();
}));
arrowNav($('.demo-files'), '[role="tab"]');

const modal = $('#demo-modal');
modal.addEventListener('click', () => {
  const w = demoWindow();
  if (!w) return;
  const on = modal.getAttribute('aria-pressed') !== 'true';
  modal.setAttribute('aria-pressed', String(on));
  modal.textContent = `Modal: ${on ? 'on' : 'off'}`;
  w.dwcodeSetMode(on);
  w.dwcodeEditor.focus();
});

// Themes: swatches for every palette, and the demo's theme list
let themes = [];
fetch('data/themes.json').then((r) => r.json()).then((list) => {
  themes = list;
  $('#theme-count').textContent = list.length;
  const ul = $('#swatches');
  for (const t of list) {
    const li = document.createElement('li');
    const b = document.createElement('button');
    b.className = 'swatch';
    b.type = 'button';
    b.dataset.id = t.id;
    b.title = `${t.name} (${t.appearance})`;
    const chips = document.createElement('div');
    chips.className = 'chips';
    for (const c of [t.surfaces.editor, t.accents.primary, t.accents.secondary, t.syntax.string, t.syntax.number]) {
      const i = document.createElement('i');
      i.style.background = c;
      chips.append(i);
    }
    const name = document.createElement('span');
    name.textContent = t.name;
    b.append(chips, name);
    b.addEventListener('click', () => {
      startDemo();
      applyTheme(t.id);
      $('#demo').scrollIntoView({ block: 'start' });
    });
    li.append(b);
    ul.append(li);
    select.append(new Option(t.name, t.id));
  }
  select.value = 'cyberpunk-chrome';
}).catch(() => { $('.swatches-head p').textContent = 'Theme list unavailable right now.'; });

// Copy buttons
$$('[data-copy]').forEach((btn) => btn.addEventListener('click', async () => {
  try {
    await navigator.clipboard.writeText($(`#${btn.dataset.copy}`).textContent.trim());
    btn.textContent = 'Copied';
  } catch {
    btn.textContent = 'Select & copy';
  }
  setTimeout(() => { btn.textContent = 'Copy'; }, 1600);
}));
