import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const projectRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const packRoot = path.join(projectRoot, "packs", "dwcode-core");
const manifestPath = path.join(packRoot, "manifest.json");
const fail = (message) => {
  console.error(`DWCode pack check failed: ${message}`);
  process.exitCode = 1;
};

function readJson(relativePath) {
  const absolutePath = path.resolve(packRoot, relativePath);
  if (!absolutePath.startsWith(`${packRoot}${path.sep}`)) {
    throw new Error(`resource escapes the pack directory: ${relativePath}`);
  }
  return JSON.parse(fs.readFileSync(absolutePath, "utf8"));
}

try {
  const manifest = JSON.parse(fs.readFileSync(manifestPath, "utf8"));
  if (manifest.manifestVersion !== 1) fail("manifestVersion must be 1");
  if (manifest.id !== "cybercore.dwcode-core") fail("unexpected pack id");
  if (manifest.kind !== "declarative") fail("the starter pack must remain declarative");
  if (!manifest.license) fail("license metadata is required, including an explicit pending-license state");
  if (manifest.main || manifest.browser) fail("declarative packs cannot contain executable entry points");
  if ((manifest.capabilities ?? []).length !== 0) fail("the data-only starter pack must request no capabilities");

  const themes = manifest.contributes?.themes ?? [];
  const tokenGroups = ["surfaces", "text", "accents", "syntax"];
  let referenceKeys;
  for (const relativePath of themes) {
    const theme = readJson(relativePath);
    if (theme.schemaVersion !== 1) fail(`${relativePath} must use schemaVersion 1`);
    if (!theme.id || !theme.name) fail(`${relativePath} must have an id and name`);
    for (const group of tokenGroups) {
      const tokens = theme[group];
      if (!tokens || typeof tokens !== "object") {
        fail(`${relativePath} is missing ${group} tokens`);
        continue;
      }
      const keys = Object.keys(tokens).sort().join(",");
      if (referenceKeys?.[group] && referenceKeys[group] !== keys) {
        fail(`${relativePath} has a different ${group} token set`);
      }
      referenceKeys ??= {};
      referenceKeys[group] ??= keys;
      for (const [token, color] of Object.entries(tokens)) {
        if (!/^#[0-9a-f]{6}$/i.test(color)) fail(`${relativePath} ${group}.${token} is not a 6-digit hex color`);
      }
    }
    if (!["dark", "light"].includes(theme.appearance)) fail(`${relativePath} has an unsupported appearance`);
  }

  const iconTheme = readJson(manifest.contributes?.iconTheme);
  for (const [iconId, iconPath] of Object.entries(iconTheme.icons ?? {})) {
    const absolutePath = path.resolve(path.dirname(path.join(packRoot, manifest.contributes.iconTheme)), iconPath);
    if (!absolutePath.startsWith(`${packRoot}${path.sep}`) || !fs.existsSync(absolutePath)) {
      fail(`icon ${iconId} points to a missing or out-of-pack asset`);
      continue;
    }
    const svg = fs.readFileSync(absolutePath, "utf8");
    if (!svg.startsWith("<svg") || /<script\b|<foreignObject\b|(?:href|src)\s*=\s*["']https?:/i.test(svg)) {
      fail(`icon ${iconId} is not a simple local SVG asset`);
    }
  }

  for (const [language, relativePath] of Object.entries(manifest.contributes?.snippets ?? {})) {
    const snippets = readJson(relativePath);
    for (const [name, snippet] of Object.entries(snippets)) {
      if (!snippet.prefix || !Array.isArray(snippet.body) || !snippet.description) {
        fail(`${language} snippet ${name} needs a prefix, body array, and description`);
      }
    }
  }

  if (!process.exitCode) console.log(`Validated ${themes.length} DWCode themes, ${Object.keys(iconTheme.icons).length} icon assets, and ${Object.keys(manifest.contributes.snippets).length} snippet groups.`);
} catch (error) {
  fail(error.message);
}
