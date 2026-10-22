# DWCode logo: Bracket / Monaspace Krypton

Chosen 2026-10-21. Drawn chamfered brackets frame the name; letters are
Monaspace Krypton ExtraBold (800). Cyan D `#2ee6f6`, magenta W `#ff2e97`,
steel CODE `#e3eaf0`, brackets `#5f6f82`, block cursor cyan.

Rebuild everything with `python3 tools/build-logo-assets.py` (after `npm ci`).

| Folder | Contents |
|---|---|
| `svg/` | outlined SVGs: `wordmark`, `icon`, `lockup` in `dark`, `light`, `mono`; `icon-dark-notile`, `icon-maskable`, `favicon.svg`, `social.svg` |
| `svg/editable/` | live-text SVGs that use the font in `font/` — edit the text in Inkscape/Figma (install the font first) |
| `png/icon/` | app icon 16–1024 px (≤48 px uses the bolder favicon drawing) |
| `png/icon-transparent/` | icon without its tile, 64–1024 px |
| `png/wordmark/`, `png/lockup/` | 256, 512, 1024, 2048 px wide in dark, light and mono |
| `png/app/` | apple-touch-icon 180, android-chrome 192/512, maskable 512 |
| `png/social/` | 1200×630 social card |
| `ico/favicon.ico` | 16, 24, 32, 48, 64, 128, 256 px |
| `webp/` | web-ready lockup, wordmark and icon |
| `font/` | Monaspace Krypton 800 (woff2, woff) and its licence |

Use `dark` on dark backgrounds, `light` on light ones, `mono` for single-colour
printing or embossing.

## Font licence

Monaspace is © 2023 GitHub, under the SIL Open Font License 1.1 with the
Reserved Font Names "Monaspace" and "Krypton" (see `font/OFL-Monaspace.txt`).
Using the font unmodified, as here, is fine. If you edit the glyphs, the
modified font must be released under a different name (e.g. "DWCode Mono").
Font sources: https://github.com/githubnext/monaspace

The logo artwork itself is © Cybercore Tech; this pack grants no licence to it.
