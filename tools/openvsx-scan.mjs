#!/usr/bin/env node
// DWCode catalog licence gate: batch-check Open VSX packages against the rules in
// FOSS_EXTENSION_REPOSITORY.md before they enter the curated index.
//
//   node tools/openvsx-scan.mjs <ns.name> [<ns.name> …]   # check explicit packages
//   node tools/openvsx-scan.mjs --keywords python,rust    # top-N sweep per keyword
//   node tools/openvsx-scan.mjs --doc                     # re-check every id in the shortlist
//   node tools/openvsx-scan.mjs --doc --json              # machine-readable report
//   node tools/openvsx-scan.mjs --doc --appendix          # markdown table of eligible rows (doc)
//   node tools/openvsx-scan.mjs --doc --site              # HTML rows for site/repository.html
//
// Verdicts:
//   accept    registry licence is a known FOSS/OSI licence          -> eligible for the catalog
//   verified  registry field is a pointer, but the upstream LICENSE  -> checked by hand in
//             file was read and recorded in openvsx-verified.json      tools/openvsx-verified.json
//   verify    empty, NOASSERTION, SEE LICENSE IN, LicenseRef, other  -> stop; check upstream first
//   refuse    a licence we recognise as non-FOSS (proprietary/EULA)  -> never index
//   missing   404 on Open VSX                                        -> out of reach as a package
//
// Exit code 1 while any `verify` verdict is pending — that is the human's job. `refuse` and
// `missing` are decisions already recorded in the shortlist, so they are reported but do not
// block a release; a package delisting itself is still printed, because that is news.

import { readFile } from 'node:fs/promises';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';

const API = 'https://open-vsx.org/api';
const HERE = dirname(fileURLToPath(import.meta.url));
const DOC = join(HERE, '..', 'FOSS_EXTENSION_REPOSITORY.md');
// Hand-checked upstream licences (see the file's _comment).
const VERIFIED = JSON.parse(await readFile(join(HERE, 'openvsx-verified.json'), 'utf8'));
const OK = new Set(['accept', 'verified']);

// Licences accepted without further review: permissive, weak copyleft, and GPL-family
// packages that DWCode only ever consumes as data or as an external process.
const ACCEPT = new Set([
  'MIT', 'APACHE-2.0', 'APACHE 2.0', 'BSD-2-CLAUSE', 'BSD-3-CLAUSE', 'BSD', 'ISC', 'UNLICENSE',
  'CC0-1.0', 'ZLIB', '0BSD', 'WTFPL', 'MPL-2.0', 'EPL-2.0', 'NCSA', 'BSL-1.0', 'PYTHON-2.0',
  'RUBY', 'AFL-3.0', 'ARTISTIC-2.0', 'MS-PL', 'MS-RL', 'LGPL-2.1', 'LGPL-2.1-OR-LATER',
  'LGPL-3.0', 'LGPL-3.0-OR-LATER', 'GPL-2.0', 'GPL-2.0-OR-LATER', 'GPL-3.0', 'GPL-3.0-OR-LATER',
  'AGPL-3.0', 'AGPL-3.0-OR-LATER', 'MIT OR APACHE-2.0', 'APACHE-2.0 OR MIT',
  'MIT OR GPL-2.0', 'APACHE-2.0 OR GPL-3.0', 'CC-BY-4.0', 'CC-BY-SA-4.0',
]);
// Named non-FOSS licences: the source-available / source-availability traps.
const REFUSE = new Set(['UNLICENSED', 'SSPL-1.0', 'BUSL-1.1', 'BUSL', 'ELV2', 'SEE LICENSE']);

const norm = (s) => String(s ?? '').trim().toUpperCase().replace(/\s+/g, ' ');
const isPointer = (s) => /^(NOASSERTION|NONE|SEE LICENSE IN|LICENSEREF|%|N\/A|$)/i.test(String(s ?? '').trim());

const get = async (url, tries = 3) => {
  for (let i = 0; i < tries; i++) {
    try {
      const r = await fetch(url, { headers: { 'user-agent': 'dwcode-catalog-gate' } });
      if (r.status === 404) return { status: 404 };
      if (r.ok) return { status: 200, body: await r.json() };
      if (r.status !== 429) return { status: r.status };
    } catch { /* retry */ }
    await new Promise((r) => setTimeout(r, 400 * (i + 1)));
  }
  return { status: 0 };
};

function verdict(license) {
  const n = norm(license);
  if (isPointer(license)) return 'verify';
  if (REFUSE.has(n) || n.includes('SEE LICENSE') || n.includes('EULA')) return 'refuse';
  if (ACCEPT.has(n)) return 'accept';
  if (/^(MIT|APACHE|BSD|GPL|LGPL|AGPL|MPL|EPL|ISC|UNLICENSE|CC0|NCSA|ZLIB)[ -]/.test(n)) return 'accept';
  return 'verify';
}

async function lookup(id) {
  const i = id.lastIndexOf('.');
  if (i < 1) return { id, verdict: 'verify', license: '(not an id)', downloads: 0, repository: '' };
  const { status, body } = await get(`${API}/${id.slice(0, i)}/${id.slice(i + 1)}`);
  if (status === 404) return { id, verdict: 'missing', license: '404', downloads: 0, repository: '' };
  if (!body) return { id, verdict: 'verify', license: '(api error)', downloads: 0, repository: '' };
  let v = verdict(body.license);
  const license = body.license || '(empty)';
  // A registry pointer is not a grant — but a hand-verified upstream reading is.
  const hand = VERIFIED[id];
  if (v === 'verify' && hand) v = verdict(hand.spdx) === 'accept' ? 'verified' : 'refuse';
  return {
    id,
    verdict: v,
    name: body.displayName || body.name || '',
    license: v === 'verified' ? `${license} → ${hand.spdx} (verified ${hand.checked})` : license,
    downloads: body.downloadCount ?? 0,
    repository: (body.repository || '').replace(/^git\+/, '').replace(/\.git$/, ''),
    version: body.version,
  };
}

async function sweep(keywords, size) {
  const rows = [];
  for (const kw of keywords) {
    const { body } = await get(`${API}/-/search?query=${encodeURIComponent(kw)}&size=${size}&orderBy=downloadCount&sortOrder=desc`);
    for (const e of body?.extensions ?? []) {
      rows.push(await lookup(`${e.namespace}.${e.name}`));
      await new Promise((r) => setTimeout(r, 80));
    }
    console.error(`// swept "${kw}"`);
  }
  return rows;
}

// Backticked tokens in the shortlist that look like an extension id: exactly one dot,
// no path separators. File names (`session.json`, `packloader.cpp`), capability names
// (`network.connect`) and bare domains (`gitlab.com`) are filtered out; anything that
// survives and does not exist simply reports `404 / missing`.
const FILE_WORDS = /\.(JSON|JS|CPP|H|MJS|MD|QML|SH|PY|TS|CSS|HTML|XML|PRO|PNG|WEBP|SVG|TXT|YML|YAML|TOML|LOCK|ICO|SQL|SCSS|WS|LIST|GZ|ZIP|JL)$/i;
const NOT_IDS = /^(NETWORK|PROCESS|WORKSPACE|CONTRIBUTES|FILES|STORAGE|UI)\./i;
const TLDS = /\.(COM|ORG|NET|DEV|IO|APP|SH|DEV)$/i;
async function idsFromDoc() {
  const text = await readFile(DOC, 'utf8');
  const found = new Set();
  for (const [, a, b] of text.matchAll(/`([A-Za-z0-9][\w-]*)\.([A-Za-z0-9][\w-]*)`/g)) {
    const id = `${a}.${b}`;
    if (FILE_WORDS.test(id) || NOT_IDS.test(id) || TLDS.test(id) || a.includes('_')) continue;
    found.add(id);
  }
  return [...found];
}

// ------------------------------------------------------------------ cli
const argv = process.argv.slice(2);
const json = argv.includes('--json');
const asDoc = argv.includes('--doc');
const kwIdx = argv.indexOf('--keywords');
const sizeIdx = argv.indexOf('--size');
const size = sizeIdx >= 0 ? Number(argv[sizeIdx + 1]) : 12;
let targets = argv.filter((a, i) => !a.startsWith('--')
  && (kwIdx < 0 || i !== kwIdx + 1) && (sizeIdx < 0 || i !== sizeIdx + 1));

let rows;
if (kwIdx >= 0) {
  const kws = (targets.length ? targets.join(',') : argv[kwIdx + 1] || '').split(',').filter(Boolean);
  if (!kws.length) { console.error('usage: --keywords python,rust'); process.exit(2); }
  rows = await sweep(kws, size);
} else {
  if (!targets.length && asDoc) targets = await idsFromDoc();
  if (!targets.length) { console.error('usage: openvsx-scan.mjs <ns.name>… | --keywords a,b | --doc'); process.exit(2); }
  rows = [];
  for (const id of targets) { rows.push(await lookup(id)); await new Promise((r) => setTimeout(r, 80)); }
}

rows.sort((a, b) => b.downloads - a.downloads);
const pending = rows.filter((r) => r.verdict === 'verify');
const eligible = rows.filter((r) => OK.has(r.verdict));
const excluded = rows.filter((r) => r.verdict === 'refuse');
const absent = rows.filter((r) => r.verdict === 'missing');

// Human-facing licence for a table cell: the registry pointer is noise once it has been
// resolved, so `SEE LICENSE IN LICENSE.md → MIT (verified 2026-10-26)` renders as `MIT`.
function shortLicense(r) {
  const arrow = r.license.indexOf(' → ');
  if (r.verdict === 'verified' && arrow >= 0) return r.license.slice(arrow + 3).replace(/ \(verified .*$/, '');
  return r.license;
}
const millions = (n) => (n >= 1e6 ? `${(n / 1e6).toFixed(2).replace(/\.?0+$/, '')} M` : n >= 1e3 ? `${Math.round(n / 1e3)} K` : String(n));
const esc = (s) => String(s).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
const uplink = (r) => (r.repository.startsWith('http') ? `[upstream](${r.repository})` : '—');

// --appendix: a markdown table of every catalog-eligible package, for the doc.
// --site:     the same rows as HTML <tr> cells for site/repository.html.
// Both read the same data as --json, so the published lists cannot drift from the gate.
if (argv.includes('--appendix')) {
  console.log('| Package | Licence | Downloads | Upstream |');
  console.log('| --- | --- | --- | --- |');
  for (const r of eligible) console.log(`| ${esc(r.name || r.id)} (\`${r.id}\`) | ${esc(shortLicense(r))} | ${millions(r.downloads)} | ${uplink(r)} |`);
  console.error(`// appendix: ${eligible.length} catalog-eligible packages`);
} else if (argv.includes('--site')) {
  for (const r of eligible) {
    console.log(`            <tr><td>${esc(r.name || r.id)}<br><code>${esc(r.id)}</code></td><td>${esc(shortLicense(r))}</td><td>${millions(r.downloads)}</td><td>${r.repository.startsWith('http') ? `<a href="${esc(r.repository)}" target="_blank" rel="noopener">upstream</a>` : '—'}</td></tr>`);
  }
  console.error(`// site table: ${eligible.length} rows`);
} else if (json) {
  console.log(JSON.stringify({ checked: rows.length, eligible: eligible.length, pending: pending.length, excluded: excluded.length, missing: absent.length, rows }, null, 2));
} else {
  const pad = (s, n) => String(s).padEnd(n);
  const w = Math.max(26, ...rows.map((r) => r.license.length + 2));
  console.log(`${pad('VERDICT', 9)}${pad('ID', 40)}${pad('DOWNLOADS', 12)}${pad('LICENSE', w)}REPOSITORY`);
  for (const r of rows) {
    console.log(`${pad(r.verdict, 9)}${pad(r.id, 40)}${pad(r.downloads, 12)}${pad(r.license, w)}${r.repository}`);
  }
  console.error(`// ${rows.length} checked · ${eligible.length} catalog-eligible · ${pending.length} pending · ${excluded.length} refused · ${absent.length} not on Open VSX`);
  if (pending.length) console.error(`// pending: ${pending.map((r) => r.id).join(', ')}`);
}
process.exit(pending.length ? 1 : 0);
