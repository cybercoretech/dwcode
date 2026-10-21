#!/usr/bin/env python3
"""Export the inline logo concepts as SVG/PNG assets and compose the board."""

from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BRAND = ROOT / "brand"
OPTIONS = BRAND / "options"
HTML = (BRAND / "logo-selection.html").read_text()
BASE = BRAND / "dwcode-logo-board-base.png"
BOARD = BRAND / "dwcode-logo-options.png"
OPTIONS.mkdir(exist_ok=True)

start = HTML.index("const logos=[")
end = HTML.index("\n    ];", start)
entries = HTML[start:end]
found = re.findall(r'\{name:"([^"]+)".*?svg:`(.*?)`\}', entries, re.S)
if len(found) != 10:
    raise SystemExit(f"Expected 10 SVG concepts, found {len(found)}")

xs = [-10.7, -5.35, 0, 5.35, 10.7]
ys = [3.25, -3.95]
current = BASE
for index, (name, source) in enumerate(found):
    slug = re.sub(r"[^a-z0-9]+", "-", name.lower()).strip("-")
    svg_path = OPTIONS / f"{index + 1:02d}-{slug}.svg"
    png_path = OPTIONS / f"{index + 1:02d}-{slug}.png"
    svg_path.write_text(source + "\n")
    subprocess.run(["rsvg-convert", "-w", "480", "-h", "480", "-o", str(png_path), str(svg_path)], check=True)
    subprocess.run(["magick", str(png_path), "-resize", "300x300", "-strip", str(png_path)], check=True)

    row, col = divmod(index, 5)
    world_x = xs[col]
    world_y = ys[row] + 0.7
    center_x = round((world_x + 14) / 28 * 2400)
    center_y = round((9.333333 - world_y) / 18.666667 * 1600)
    left, top = center_x - 150, center_y - 150
    nxt = BRAND / f".logo-board-{index + 1:02d}.png"
    subprocess.run([
        "magick", str(current), str(png_path), "-geometry", f"+{left}+{top}",
        "-compose", "over", "-composite", str(nxt),
    ], check=True)
    if current != BASE and current.exists():
        current.unlink()
    current = nxt

subprocess.run(["magick", str(current), "-strip", "-define", "png:compression-level=9", str(BOARD)], check=True)
if current != BASE:
    current.unlink(missing_ok=True)
print(f"Rendered {len(found)} concepts to {OPTIONS} and {BOARD}")
