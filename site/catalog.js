// Extension catalog interaction for /repository: search, family filter, sort,
// live count, empty state and copy-id feedback.
//
// The card grid itself is static HTML written by `node tools/catalog.mjs render`
// (data from `node tools/catalog.mjs build`), so the catalog is readable and
// crawlable with JavaScript off. This file only adds the marketplace controls
// on top — it never fetches package data at runtime.

const grid = document.getElementById('xgrid');
if (grid) {
  const cards = [...grid.querySelectorAll('.xcard')];
  const q = document.getElementById('cat-q');
  const sort = document.getElementById('cat-sort');
  const count = document.getElementById('cat-count');
  const empty = document.getElementById('cat-empty');
  const cats = [...document.querySelectorAll('.cat')];
  let family = 'all';

  // Sort keys are already on each card as data attributes (set at render time),
  // so ordering never re-reads text out of the DOM.
  const orders = {
    dl: (a, b) => +b.dataset.dl - +a.dataset.dl || a.dataset.name.localeCompare(b.dataset.name),
    name: (a, b) => a.dataset.name.localeCompare(b.dataset.name),
    pub: (a, b) => a.dataset.pub.localeCompare(b.dataset.pub) || a.dataset.name.localeCompare(b.dataset.name),
    lic: (a, b) => a.dataset.lic.localeCompare(b.dataset.lic) || +b.dataset.dl - +a.dataset.dl,
  };

  function apply() {
    const needle = (q?.value || '').trim().toLowerCase();
    let shown = 0;
    for (const card of cards) {
      const hit = (family === 'all' || card.dataset.family === family)
        && (!needle
          || card.dataset.name.includes(needle)
          || card.dataset.pub.includes(needle)
          || card.dataset.id.toLowerCase().includes(needle)
          || card.dataset.lic.includes(needle));
      card.hidden = !hit;
      if (hit) shown += 1;
    }
    if (count) count.textContent = `${shown} shown`;
    if (empty) empty.hidden = shown !== 0;
  }

  function reorder() {
    const order = orders[sort?.value] || orders.dl;
    for (const card of [...cards].sort(order)) grid.append(card);
  }

  for (const btn of cats) {
    btn.addEventListener('click', () => {
      family = btn.dataset.family;
      for (const other of cats) {
        const on = other === btn;
        other.classList.toggle('is-on', on);
        other.setAttribute('aria-pressed', String(on));
      }
      apply();
    });
  }

  q?.addEventListener('input', apply);
  sort?.addEventListener('change', reorder);

  // "/" is the search key every code tool uses; ignore it while typing elsewhere.
  addEventListener('keydown', (e) => {
    const inField = /^(input|textarea|select)$/i.test(e.target?.tagName || '') || e.target?.isContentEditable;
    if (e.key === '/' && !inField) { e.preventDefault(); q?.focus(); q?.select(); }
    if (e.key === 'Escape' && e.target === q && q.value) { q.value = ''; apply(); }
  });

  // Copy id: the one action a card offers until M5 publishes the index.
  grid.addEventListener('click', async (e) => {
    const btn = e.target.closest('[data-copy-id]');
    if (!btn) return;
    const id = btn.dataset.copyId;
    let ok = false;
    try {
      await navigator.clipboard.writeText(id);
      ok = true;
    } catch {
      // Clipboard API needs a secure context; fall back to a hidden textarea.
      try {
        const ta = document.createElement('textarea');
        ta.value = id;
        ta.setAttribute('readonly', '');
        ta.style.cssText = 'position:fixed;opacity:0;top:0';
        document.body.append(ta);
        ta.select();
        ok = document.execCommand('copy');
        ta.remove();
      } catch { ok = false; }
    }
    btn.textContent = ok ? 'copied ✓' : 'copy failed';
    setTimeout(() => { btn.textContent = 'copy id'; }, 1600);
  });

  reorder();
  apply();
}
