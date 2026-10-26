#!/usr/bin/env node
// Emits the theme catalog DWCode actually ships: the built-in list plus every
// pack contribution, de-duplicated by id in the same order the app's
// WorkspaceBackend::rebuildThemes() uses. The site gallery reads this so it
// shows the same themes as the desktop app, including the ones that now live
// in packs/dwcode-core instead of src/data/themes.json.
import { readFileSync, readdirSync } from "node:fs";
import { join, dirname } from "node:path";
import { fileURLToPath } from "node:url";

const root = join(dirname(fileURLToPath(import.meta.url)), "..");
const themes = JSON.parse(readFileSync(join(root, "src", "data", "themes.json"), "utf8"));
const seen = new Set(themes.map((theme) => theme.id));
const packsRoot = join(root, "packs");

for (const entry of readdirSync(packsRoot, { withFileTypes: true }).sort((a, b) => a.name.localeCompare(b.name))) {
  if (!entry.isDirectory()) continue;
  const directory = join(packsRoot, entry.name);
  const manifest = JSON.parse(readFileSync(join(directory, "manifest.json"), "utf8"));
  for (const relative of manifest.contributes?.themes ?? []) {
    const theme = JSON.parse(readFileSync(join(directory, relative), "utf8"));
    if (!theme.id || seen.has(theme.id)) continue;
    seen.add(theme.id);
    themes.push(theme);
  }
}

process.stdout.write(`${JSON.stringify(themes, null, 2)}\n`);
