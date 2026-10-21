#!/usr/bin/env python3
"""Builds the DWCode Bracket logo pack (Monaspace Krypton ExtraBold).

Letters are outlined from the font with fontTools so every SVG/PNG renders
the same without the font installed; "-text" SVGs keep live text for editing.

Outputs:
  brand/dwcode-bracket/   full pack (svg, png sizes, ico, webp, social, font + licence)
  site/assets/            what the website uses (favicons, icons, logo svg, social card, font)
  src/assets/             what the desktop app uses (top-bar mark, window icon)

Needs: python3 + fonttools + brotli, rsvg-convert, ImageMagick (magick),
and node_modules/@fontsource/monaspace-krypton (npm ci).
"""
import shutil, subprocess, hashlib
from pathlib import Path
from fontTools.ttLib import TTFont
from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.transformPen import TransformPen

ROOT = Path(__file__).resolve().parent.parent
KRYPTON = ROOT / "node_modules/@fontsource/monaspace-krypton/files/monaspace-krypton-latin-800-normal.woff2"
KRYPTON_WOFF = KRYPTON.with_suffix(".woff")
KRYPTON_LICENSE = ROOT / "node_modules/@fontsource/monaspace-krypton/LICENSE"
JBMONO = Path("/usr/share/fonts/TTF/JetBrainsMonoNerdFont-Regular.ttf")
PACK = ROOT / "brand/dwcode-bracket"
SITE = ROOT / "site/assets"
APP = ROOT / "src/assets"

PALETTES = {
    "dark":  dict(d="#2ee6f6", w="#ff2e97", rest="#e3eaf0", bracket="#5f6f82", cursor="#2ee6f6", tile="#0b1018", border="#223044"),
    "light": dict(d="#0a8fa0", w="#d4146f", rest="#17232b", bracket="#8a98a6", cursor="#0a8fa0", tile="#f3f6f8", border="#c9d5da"),
    "mono":  dict(d="#ffffff", w="#ffffff", rest="#ffffff", bracket="#ffffff", cursor="#ffffff", tile="#000000", border="#000000"),
}


class Outliner:
    """Text -> SVG path data for one font, placed by cap height."""
    def __init__(self, path):
        self.font = TTFont(path)
        self.upm = self.font["head"].unitsPerEm
        self.cap = self.font["OS/2"].sCapHeight
        self.cmap = self.font.getBestCmap()
        self.glyphs = self.font.getGlyphSet()

    def advance(self, ch, cap_px):
        return self.font["hmtx"][self.cmap[ord(ch)]][0] * cap_px / self.cap

    def glyph(self, ch, x, baseline, cap_px):
        """Path for one char with its origin at (x, baseline), caps cap_px tall."""
        k = cap_px / self.cap
        pen = SVGPathPen(self.glyphs)
        self.glyphs[self.cmap[ord(ch)]].draw(TransformPen(pen, (k, 0, 0, -k, x, baseline)))
        return pen.getCommands()

    def run(self, text, x, baseline, cap_px, tracking=0.0):
        out = []
        for ch in text:
            if ch != " ":
                out.append((ch, self.glyph(ch, x, baseline, cap_px)))
            x += self.advance(ch, cap_px) + tracking
        return out, x


K = Outliner(KRYPTON)


def bracket(side, x, h, top=0.0):
    w = h * 0.2
    if side == "L":
        return f"M{x+w:.2f} {top-h*.08:.2f}H{x+w*.45:.2f}L{x:.2f} {top+h*.12:.2f}V{top+h*.88:.2f}L{x+w*.45:.2f} {top+h*1.08:.2f}H{x+w:.2f}"
    return f"M{x-w:.2f} {top-h*.08:.2f}H{x-w*.45:.2f}L{x:.2f} {top+h*.12:.2f}V{top+h*.88:.2f}L{x-w*.45:.2f} {top+h*1.08:.2f}H{x-w:.2f}"


def letter_fill(i, p):
    return p["d"] if i == 0 else p["w"] if i == 1 else p["rest"]


def wordmark_parts(p, word="DWCODE", x0=0.0, H=100.0, live=False):
    """[ DWCODE ▌ ] with caps H tall between y=0 and y=H. Returns (svg body, width)."""
    parts = [f'<path d="{bracket("L", x0, H)}" fill="none" stroke="{p["bracket"]}" stroke-width="8" class="br br-l"/>']
    tx = x0 + 34
    if live:
        spans = "".join(f'<tspan fill="{letter_fill(i, p)}">{c}</tspan>' for i, c in enumerate(word))
        font_size = H * K.upm / K.cap
        parts.append(f'<text x="{tx:.2f}" y="{H}" font-family="Monaspace Krypton" font-weight="800" font-size="{font_size:.2f}">{spans}</text>')
        end = tx + sum(K.advance(c, H) for c in word)
    else:
        glyphs, end = K.run(word, tx, H, H)
        parts += [f'<path d="{d}" fill="{letter_fill(i, p)}" class="ch ch-{i}"/>' for i, (_, d) in enumerate(glyphs)]
    end += 14
    parts.append(f'<rect x="{end:.2f}" y="0" width="{H*.2:.2f}" height="{H}" fill="{p["cursor"]}" class="cursor"/>')
    rb = end + H * .2 + 30
    parts.append(f'<path d="{bracket("R", rb, H)}" fill="none" stroke="{p["bracket"]}" stroke-width="8" class="br br-r"/>')
    return "\n".join(parts), rb + 4


def icon_parts(p, tile=True, x0=0.0, y0=0.0):
    """144x144 icon: tile, [ DW ] and an underscore cursor."""
    out = []
    if tile:
        out.append(f'<rect x="{x0+1}" y="{y0+1}" width="142" height="142" rx="30" fill="{p["tile"]}" stroke="{p["border"]}" stroke-width="2"/>')
    bh = 64
    out.append(f'<path d="{bracket("L", x0+22, bh, y0+40)}" fill="none" stroke="{p["bracket"]}" stroke-width="7" class="br"/>')
    out.append(f'<path d="{bracket("R", x0+122, bh, y0+40)}" fill="none" stroke="{p["bracket"]}" stroke-width="7" class="br"/>')
    cap = 46.0
    width = K.advance("D", cap) * 2 - 2
    x = x0 + 72 - width / 2
    for i, ch in enumerate("DW"):
        out.append(f'<path d="{K.glyph(ch, x, y0+49+cap, cap)}" fill="{letter_fill(i, p)}"/>')
        x += K.advance(ch, cap) - 2
    out.append(f'<rect x="{x0+100}" y="{y0+118}" width="18" height="6" fill="{p["cursor"]}" class="cursor"/>')
    return "\n".join(out)


def svg(body, w, h, vx=0, vy=0, title="DWCode", extra_defs=""):
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="{vx:.2f} {vy:.2f} {w:.2f} {h:.2f}" width="{w:.0f}" height="{h:.0f}" role="img" aria-label="{title}">'
            f"<title>{title}</title>{extra_defs}\n{body}\n</svg>\n")


def font_face(rel):
    return (f'<defs><style>@font-face{{font-family:"Monaspace Krypton";font-weight:800;'
            f'src:url("{rel}monaspace-krypton-latin-800-normal.woff2") format("woff2")}}</style></defs>')


def write(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)


def png(src, dest, width=None, height=None):
    dest.parent.mkdir(parents=True, exist_ok=True)
    args = ["rsvg-convert", str(src), "-o", str(dest)]
    if width: args += ["-w", str(width)]
    if height: args += ["-h", str(height)]
    subprocess.run(args, check=True)


def main():
    if PACK.exists():
        shutil.rmtree(PACK)
    made = []
    for name, p in PALETTES.items():
        body, ww = wordmark_parts(p)
        write(PACK / f"svg/dwcode-wordmark-{name}.svg", svg(body, ww + 8, 136, -4, -18, "DWCode"))
        ib = icon_parts(p, tile=(name != "mono"))
        write(PACK / f"svg/dwcode-icon-{name}.svg", svg(ib, 144, 144, 0, 0, "DWCode icon"))
        lw_body, lw = wordmark_parts(p, x0=176, H=100)
        lockup = icon_parts(p, tile=(name != "mono"), x0=0, y0=-22) + "\n" + lw_body
        write(PACK / f"svg/dwcode-lockup-{name}.svg", svg(lockup, lw + 8, 152, -4, -26, "DWCode"))
        made += [f"wordmark-{name}", f"icon-{name}", f"lockup-{name}"]
    # editable (live text) versions, font shipped next to them
    body, ww = wordmark_parts(PALETTES["dark"], live=True)
    write(PACK / "svg/editable/dwcode-wordmark-dark-text.svg", svg(body, ww + 8, 136, -4, -18, "DWCode", font_face("../../font/")))
    lw_body, lw = wordmark_parts(PALETTES["dark"], x0=176, live=True)
    write(PACK / "svg/editable/dwcode-lockup-dark-text.svg", svg(icon_parts(PALETTES["dark"], x0=0, y0=-22) + "\n" + lw_body, lw + 8, 152, -4, -26, "DWCode", font_face("../../font/")))
    # transparent icon (no tile) for overlays
    write(PACK / "svg/dwcode-icon-dark-notile.svg", svg(icon_parts(PALETTES["dark"], tile=False), 144, 144, 0, 0, "DWCode icon"))

    # favicon.svg: simplified for tiny sizes (no brackets below ~24 px look muddy; keep them, thicker)
    p = PALETTES["dark"]
    fav = (f'<rect x="1" y="1" width="142" height="142" rx="30" fill="{p["tile"]}"/>'
           f'<path d="{bracket("L", 16, 74, 35)}" fill="none" stroke="{p["bracket"]}" stroke-width="10"/>'
           f'<path d="{bracket("R", 128, 74, 35)}" fill="none" stroke="{p["bracket"]}" stroke-width="10"/>')
    cap = 56.0
    x = 72 - (K.advance("D", cap) * 2 - 4) / 2
    for i, ch in enumerate("DW"):
        fav += f'<path d="{K.glyph(ch, x, 44+cap, cap)}" fill="{letter_fill(i, p)}"/>'
        x += K.advance(ch, cap) - 4
    write(PACK / "svg/favicon.svg", svg(fav, 144, 144, 0, 0, "DWCode"))

    # PNG sizes
    for size in (16, 24, 32, 48, 64, 128, 256, 512, 1024):
        png(PACK / ("svg/favicon.svg" if size <= 48 else "svg/dwcode-icon-dark.svg"), PACK / f"png/icon/dwcode-icon-{size}.png", size, size)
    for size in (64, 128, 256, 512, 1024):
        png(PACK / "svg/dwcode-icon-dark-notile.svg", PACK / f"png/icon-transparent/dwcode-icon-notile-{size}.png", size, size)
    for name in PALETTES:
        for width in (256, 512, 1024, 2048):
            png(PACK / f"svg/dwcode-wordmark-{name}.svg", PACK / f"png/wordmark/dwcode-wordmark-{name}-{width}.png", width)
            png(PACK / f"svg/dwcode-lockup-{name}.svg", PACK / f"png/lockup/dwcode-lockup-{name}-{width}.png", width)
    png(PACK / "svg/dwcode-icon-dark.svg", PACK / "png/app/apple-touch-icon.png", 180, 180)
    for size in (192, 512):
        png(PACK / "svg/dwcode-icon-dark.svg", PACK / f"png/app/android-chrome-{size}.png", size, size)
    # maskable: icon content inside the 80% safe zone on a full-bleed tile
    mask = (f'<rect width="144" height="144" fill="{p["tile"]}"/>'
            f'<g transform="translate(14.4 14.4) scale(.8)">{icon_parts(p, tile=False)}</g>')
    write(PACK / "svg/dwcode-icon-maskable.svg", svg(mask, 144, 144, 0, 0, "DWCode"))
    png(PACK / "svg/dwcode-icon-maskable.svg", PACK / "png/app/maskable-512.png", 512, 512)

    # ICO + WebP
    ico_src = [str(PACK / f"png/icon/dwcode-icon-{s}.png") for s in (16, 24, 32, 48, 64, 128, 256)]
    (PACK / "ico").mkdir(parents=True, exist_ok=True)
    subprocess.run(["magick", *ico_src, str(PACK / "ico/favicon.ico")], check=True)
    (PACK / "webp").mkdir(parents=True, exist_ok=True)
    for src in ("png/lockup/dwcode-lockup-dark-1024.png", "png/wordmark/dwcode-wordmark-dark-1024.png", "png/icon/dwcode-icon-512.png"):
        subprocess.run(["magick", str(PACK / src), "-quality", "90", str(PACK / "webp" / (Path(src).stem + ".webp"))], check=True)

    # Social card 1200x630
    J = Outliner(JBMONO)
    lw_body, lw = wordmark_parts(PALETTES["dark"], x0=176, H=100)
    scale = 860 / (lw + 8)
    tag, _ = J.run("Code after dark.", 0, 0, 40)
    sub, _ = J.run("keyboard-first desktop editor · qt quick + monaco · 84 themes", 0, 0, 17)
    grid = "".join(f'<path d="M{x} 0V630" stroke="#2ee6f6" stroke-opacity=".05"/>' for x in range(0, 1201, 40)) + \
           "".join(f'<path d="M0 {y}H1200" stroke="#2ee6f6" stroke-opacity=".05"/>' for y in range(0, 631, 40))
    social = (
        '<defs><radialGradient id="c" cx="0" cy="0" r="1" gradientUnits="userSpaceOnUse" gradientTransform="translate(180 120) scale(620 420)"><stop stop-color="#2ee6f6" stop-opacity=".18"/><stop offset="1" stop-color="#2ee6f6" stop-opacity="0"/></radialGradient>'
        '<radialGradient id="m" cx="0" cy="0" r="1" gradientUnits="userSpaceOnUse" gradientTransform="translate(1080 560) scale(640 420)"><stop stop-color="#ff2e97" stop-opacity=".16"/><stop offset="1" stop-color="#ff2e97" stop-opacity="0"/></radialGradient></defs>'
        f'<rect width="1200" height="630" fill="#06080d"/>{grid}<rect width="1200" height="630" fill="url(#c)"/><rect width="1200" height="630" fill="url(#m)"/>'
        f'<g transform="translate(170 150) scale({scale:.4f}) translate(4 26)">{icon_parts(PALETTES["dark"], x0=0, y0=-22)}{lw_body}</g>'
        + "".join(f'<path d="{d}" fill="#f2f6fa" transform="translate(176 430)"/>' for _, d in tag)
        + "".join(f'<path d="{d}" fill="#7d8a99" transform="translate(178 480)"/>' for _, d in sub)
        + '<rect x="60" y="560" width="200" height="3" fill="#2ee6f6"/><rect x="940" y="560" width="200" height="3" fill="#ff2e97"/>'
    )
    write(PACK / "svg/dwcode-social.svg", svg(social, 1200, 630, 0, 0, "DWCode — Code after dark"))
    png(PACK / "svg/dwcode-social.svg", PACK / "png/social/dwcode-social-1200x630.png", 1200, 630)

    # Font + licence (unmodified; Reserved Font Name "Monaspace"/"Krypton")
    (PACK / "font").mkdir(parents=True, exist_ok=True)
    shutil.copy2(KRYPTON, PACK / "font" / KRYPTON.name)
    shutil.copy2(KRYPTON_WOFF, PACK / "font" / KRYPTON_WOFF.name)
    shutil.copy2(KRYPTON_LICENSE, PACK / "font/OFL-Monaspace.txt")

    write(PACK / "README.md", README)
    sums = []
    for f in sorted(PACK.rglob("*")):
        if f.is_file() and f.name != "SHA256SUMS":
            sums.append(f"{hashlib.sha256(f.read_bytes()).hexdigest()}  {f.relative_to(PACK)}")
    write(PACK / "SHA256SUMS", "\n".join(sums) + "\n")

    # Website assets
    SITE.mkdir(parents=True, exist_ok=True)
    for src, dest in [
        ("svg/favicon.svg", "favicon.svg"), ("ico/favicon.ico", "favicon.ico"),
        ("png/app/apple-touch-icon.png", "apple-touch-icon.png"),
        ("png/app/android-chrome-192.png", "android-chrome-192.png"), ("png/app/android-chrome-512.png", "android-chrome-512.png"),
        ("png/app/maskable-512.png", "maskable-512.png"),
        ("png/social/dwcode-social-1200x630.png", "social.png"),
        ("svg/dwcode-lockup-dark.svg", "dwcode-lockup.svg"), ("svg/dwcode-icon-dark.svg", "dwcode-icon.svg"),
        ("svg/dwcode-wordmark-dark.svg", "dwcode-wordmark.svg"),
    ]:
        (SITE / dest).parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(PACK / src, SITE / dest)
    # Desktop app assets
    shutil.copy2(PACK / "png/icon/dwcode-icon-256.png", APP / "dwcode-icon.png")
    png(PACK / "svg/dwcode-wordmark-dark.svg", APP / "dwcode-wordmark.png", height=64)
    print(f"pack: {sum(1 for f in PACK.rglob('*') if f.is_file())} files in {PACK.relative_to(ROOT)}")


README = """# DWCode logo: Bracket / Monaspace Krypton

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
"""

if __name__ == "__main__":
    main()
