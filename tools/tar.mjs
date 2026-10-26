#!/usr/bin/env node
// Minimal POSIX **ustar** writer for DWCode pack artifacts.
//
// Why hand-rolled instead of shelling out to `tar`: the artifact's sha256 is
// the thing an install trusts, so the bytes have to be reproducible on any
// machine, in CI, forever. `tar` varies by implementation, by environment
// variables (TAR_OPTIONS) and by the source files' mtimes, owners and modes.
// Writing the 512-byte headers ourselves removes every one of those variables:
// the archive is a pure function of the file names, their contents and their
// order.
//
// It deliberately supports almost nothing - that is the point:
//   * format ustar ("ustar\0" + version "00"), no GNU and no PAX extensions
//     (no 'L' long names, no 'x'/'g' extra headers), so every header a reader
//     meets is one it understands;
//   * typeflags '0' (regular file) and '5' (directory) only - no symlinks,
//     hardlinks, devices or FIFOs, so an archive can never smuggle a path out
//     of the destination directory;
//   * fixed mode/uid/gid/mtime/uname/gname/device fields - no trace of the
//     machine that built it, and no permission surprises after unpacking;
//   * relative names, at most 100 bytes, no '..' and no leading '/' - the
//     exact rules `PackStore::extractTar()` enforces on the other side.
//
// Format reference: POSIX.1-1988 ustar header, 512-byte blocks. The header
// checksum is the sum of all 512 bytes with bytes 148..155 counted as spaces,
// stored as 6 octal digits, NUL, space. An archive ends with two zero blocks.
//
//   import { packDirectory } from './tar.mjs';
//   const bytes = packDirectory('packs/dwcode-core');

import { readdirSync, statSync, readFileSync } from 'node:fs';

const BLOCK = 512;

// Octal field: `width` bytes, zero-padded, NUL-terminated. The value itself
// gets `width - 1` digits, which is what the standard reserves for it.
const octal = (value, width) => {
  const digits = value.toString(8).padStart(width - 1, '0');
  if (digits.length > width - 1) throw new Error(`tar: ${value} does not fit in a ${width}-byte octal field`);
  return Buffer.from(`${digits}\0`, 'ascii');
};

const text = (value, width) => {
  const bytes = Buffer.from(String(value), 'utf8');
  if (bytes.length > width) throw new Error(`tar: "${value}" does not fit in ${width} bytes (keep pack paths short)`);
  const buf = Buffer.alloc(width, 0);
  bytes.copy(buf);
  return buf;
};

// A pack path may not be absolute, contain a '..', '.' or empty segment, a
// control character, or exceed the 100-byte name field. Rejecting here means
// a bad path can never reach an archive that a client would then have to
// defend itself against all over again.
export function assertSafeName(name) {
  if (typeof name !== 'string' || name === '') throw new Error('tar: empty entry name');
  if (name.length > 100) throw new Error(`tar: "${name}" exceeds the 100-byte ustar name field`);
  if (name.startsWith('/')) throw new Error(`tar: "${name}" is absolute`);
  for (const segment of name.split('/')) {
    if (segment === '' || segment === '.' || segment === '..') {
      throw new Error(`tar: "${name}" contains a "${segment}" segment`);
    }
  }
  for (const ch of name) {
    const code = ch.charCodeAt(0);
    if (code < 0x20 || code === 0x7f) throw new Error(`tar: "${name}" contains a control character`);
  }
  return name;
}

const header = ({ name, size, type }) => {
  assertSafeName(name);
  const block = Buffer.alloc(BLOCK, 0);
  text(name, 100).copy(block, 0);
  octal(type === '5' ? 0o755 : 0o644, 8).copy(block, 100); // mode
  octal(0, 8).copy(block, 108);                             // uid
  octal(0, 8).copy(block, 116);                             // gid
  octal(size, 12).copy(block, 124);                         // size
  octal(0, 12).copy(block, 136);                            // mtime: fixed, no builder clock
  block.write('        ', 148, 'ascii');                    // checksum placeholder = spaces
  block.write(type, 156, 'ascii');                          // typeflag
  // linkname (157..256) stays zero: this writer never emits a link.
  block.write('ustar\0', 257, 'ascii');                     // magic
  block.write('00', 263, 'ascii');                          // version
  // uname/gname (265..328) stay zero: the archive names no owner.
  octal(0, 8).copy(block, 329);                             // devmajor
  octal(0, 8).copy(block, 337);                             // devminor
  // prefix (345..499) stays zero: every pack path fits in the name field.

  let sum = 0;
  for (const byte of block) sum += byte;
  block.write(sum.toString(8).padStart(6, '0'), 148, 'ascii');
  block.write('\0 ', 154, 'ascii');
  return block;
};

const pad = (length) => Buffer.alloc((BLOCK - (length % BLOCK)) % BLOCK, 0);

/**
 * Build an archive from `{ name, data?, dir? }` entries in the order given.
 * Directory entries become type '5' with size 0, file entries type '0', and
 * the archive ends with two 512-byte zero blocks.
 */
export function createTar(entries) {
  const out = [];
  for (const entry of entries) {
    const isDir = entry.dir === true;
    const data = isDir ? Buffer.alloc(0) : Buffer.isBuffer(entry.data) ? entry.data : Buffer.from(entry.data ?? '', 'utf8');
    out.push(header({ name: entry.name, size: data.length, type: isDir ? '5' : '0' }));
    if (data.length) {
      out.push(data);
      out.push(pad(data.length));
    }
  }
  out.push(Buffer.alloc(BLOCK * 2, 0));
  return Buffer.concat(out);
}

/** Read a directory tree into archive entries, deterministically ordered. */
export function packDirectory(dirPath) {
  const entries = [];
  const walk = (rel) => {
    const absolute = rel ? `${dirPath}/${rel}` : dirPath;
    for (const name of readdirSync(absolute).sort((a, b) => (a < b ? -1 : a > b ? 1 : 0))) {
      const childRel = rel ? `${rel}/${name}` : name;
      const childAbs = `${dirPath}/${childRel}`;
      const stat = statSync(childAbs);
      if (stat.isDirectory()) {
        entries.push({ name: childRel, dir: true });
        walk(childRel);
      } else if (stat.isFile()) {
        entries.push({ name: childRel, data: readFileSync(childAbs) });
      }
      // Anything else (symlink, socket, device) is skipped: an artifact ships
      // regular files only, and the reader refuses links regardless.
    }
  };
  walk('');
  return createTar(entries);
}
