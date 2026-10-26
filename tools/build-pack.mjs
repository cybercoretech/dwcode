#!/usr/bin/env node
// Builds the DWCode-native pack artifacts the install index points at.
//
//   node tools/build-pack.mjs            # write dist/packs/*.dwpack (+ .sha256)
//   node tools/build-pack.mjs --json     # print what would be built, write nothing
//   node tools/build-pack.mjs --check    # fail if dist/ is stale for the packs
//
// The artifact is a deterministic uncompressed ustar archive (see tools/tar.mjs
// and docs/PACK_INDEX.md §4): the same pack directory always yields the same
// bytes, so the sha256 recorded in `site/data/catalog/index.json` can be
// recomputed by CI without the artifact ever being committed to the repository.
// That is what lets the checksum be verified offline, on every build, and
// still match the file uploaded to a GitHub release.
//
// Naming: `<id>-<version>.dwpack`, e.g. `cybercore.dwcode-core-0.1.0.dwpack`.
// The release URL is derived from the tag, not guessed at: it is the same
// `v<version>` tag `tools/aur-release.sh` and `build-package.sh` use.

import { createHash } from 'node:crypto';
import { existsSync, mkdirSync, readdirSync, readFileSync, writeFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

import { packDirectory } from './tar.mjs';

const HERE = dirname(fileURLToPath(import.meta.url));
const ROOT = join(HERE, '..');
const PACKS = join(ROOT, 'packs');
const DEFAULT_OUT = join(ROOT, 'dist', 'packs');
const REPO = 'https://github.com/cybercoretech/dwcode';

const sha256 = (buf) => createHash('sha256').update(buf).digest('hex');
// Semver with an optional pre-release/build suffix — `packs/` may version a
// pack `0.2.0-rc.1` and the index has to describe it rather than refuse it.
const SEMVER = /^\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?(?:\+[0-9A-Za-z.-]+)?$/;

// Minimal manifest fields the index and the client both rely on. The full
// schema check lives in `npm run validate:packs`; here we only refuse what
// would produce a broken index entry.
const readManifest = (dir) => {
  const file = join(dir, 'manifest.json');
  if (!existsSync(file)) return null;
  const manifest = JSON.parse(readFileSync(file, 'utf8'));
  const problems = [];
  if (!manifest.id || !/^[A-Za-z0-9._-]+$/.test(manifest.id)) problems.push(`${dir}: manifest id must match [A-Za-z0-9._-]+`);
  if (!manifest.version || !SEMVER.test(manifest.version)) problems.push(`${dir}: manifest version must be X.Y.Z`);
  if (!manifest.license) problems.push(`${dir}: manifest has no license`);
  if (manifest.kind && manifest.kind !== 'declarative') problems.push(`${dir}: only declarative packs are installable, got kind "${manifest.kind}"`);
  if (problems.length) {
    console.error(`build-pack: refusing —\n  ${problems.join('\n  ')}`);
    process.exit(1);
  }
  return manifest;
};

/** One entry per installable first-party pack, with the exact bytes to publish. */
export function collectPacks() {
  if (!existsSync(PACKS)) return [];
  const packs = [];
  for (const name of readdirSync(PACKS).sort()) {
    const dir = join(PACKS, name);
    const manifest = readManifest(dir);
    if (!manifest) continue;
    const bytes = packDirectory(dir);
    packs.push({
      id: manifest.id,
      version: manifest.version,
      displayName: manifest.displayName || manifest.id,
      license: manifest.license,
      capabilities: Array.isArray(manifest.capabilities) ? manifest.capabilities : [],
      publisher: manifest.publisher || 'Cybercore Tech',
      source: dir,
      fileName: `${manifest.id}-${manifest.version}.dwpack`,
      bytes,
      size: bytes.length,
      sha256: sha256(bytes),
      url: `${REPO}/releases/download/v${manifest.version}/${manifest.id}-${manifest.version}.dwpack`,
    });
  }
  return packs;
}

function main() {
  const args = process.argv.slice(2);
  const json = args.includes('--json');
  const check = args.includes('--check');
  const out = (() => {
    const i = args.indexOf('--out');
    return i >= 0 ? args[i + 1] : DEFAULT_OUT;
  })();

  const packs = collectPacks();
  if (!packs.length) {
    console.error('build-pack: no packs under packs/ with a manifest.json');
    process.exit(1);
  }

  if (json) {
    process.stdout.write(`${JSON.stringify(packs.map(({ bytes, ...rest }) => rest), null, 2)}\n`);
    return;
  }

  if (check) {
    const problems = [];
    for (const pack of packs) {
      const file = join(out, pack.fileName);
      if (!existsSync(file)) { problems.push(`${pack.fileName} is missing — run \`node tools/build-pack.mjs\``); continue; }
      const actual = sha256(readFileSync(file));
      if (actual !== pack.sha256) problems.push(`${pack.fileName}: dist sha256 ${actual} != rebuilt ${pack.sha256} — the committed pack source changed; rebuild and re-upload`);
    }
    if (problems.length) {
      console.error(`build-pack: ${problems.length} problem(s) —\n  ${problems.join('\n  ')}`);
      process.exit(1);
    }
    console.error(`build-pack: OK — ${packs.length} artifact(s) match the source tree`);
    return;
  }

  mkdirSync(out, { recursive: true });
  for (const pack of packs) {
    const file = join(out, pack.fileName);
    writeFileSync(file, pack.bytes);
    writeFileSync(`${file}.sha256`, `${pack.sha256}  ${pack.fileName}\n`);
    console.error(`build-pack: ${pack.fileName}  ${pack.size} bytes  sha256 ${pack.sha256}`);
    console.error(`           upload as releases/download/v${pack.version}/${pack.fileName}`);
  }
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) main();
