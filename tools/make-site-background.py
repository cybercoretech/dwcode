#!/usr/bin/env python3
"""Writes site/assets/bg-editor.svg: a static, dimmed "code editor" backdrop.

Two editor panes (split view) with line numbers, syntax-coloured token bars,
indent guides, a current-line band and a minimap, under faint neon glows.
Deterministic (fixed seed), so rebuilding gives the same file.
"""
import random
from pathlib import Path

W, H = 1920, 1200
LINE = 24
rng = random.Random(0xD3C0DE)

INK = {  # syntax colour -> weight
    "#8b98a8": 46,  # identifiers / punctuation (steel)
    "#2ee6f6": 12,  # types
    "#ff2e97": 11,  # keywords
    "#9b6bff": 8,   # functions
    "#6ee7a8": 7,   # strings
    "#ffb347": 4,   # numbers
}
COLORS, WEIGHTS = zip(*INK.items())

out = []
add = out.append
add(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" preserveAspectRatio="xMidYMid slice">')
add("""<defs>
 <radialGradient id="gc" cx="0" cy="0" r="1" gradientUnits="userSpaceOnUse" gradientTransform="translate(260 140) scale(900 620)">
  <stop stop-color="#2ee6f6" stop-opacity=".16"/><stop offset="1" stop-color="#2ee6f6" stop-opacity="0"/></radialGradient>
 <radialGradient id="gm" cx="0" cy="0" r="1" gradientUnits="userSpaceOnUse" gradientTransform="translate(1700 1080) scale(1000 680)">
  <stop stop-color="#ff2e97" stop-opacity=".14"/><stop offset="1" stop-color="#ff2e97" stop-opacity="0"/></radialGradient>
 <radialGradient id="gv" cx="0" cy="0" r="1" gradientUnits="userSpaceOnUse" gradientTransform="translate(1120 420) scale(700 420)">
  <stop stop-color="#9b6bff" stop-opacity=".07"/><stop offset="1" stop-color="#9b6bff" stop-opacity="0"/></radialGradient>
 <linearGradient id="fade" x1="0" y1="0" x2="0" y2="1">
  <stop stop-color="#05070b" stop-opacity="0"/><stop offset=".75" stop-color="#05070b" stop-opacity=".35"/><stop offset="1" stop-color="#05070b" stop-opacity=".85"/></linearGradient>
 <linearGradient id="cur" x1="0" y1="0" x2="1" y2="0">
  <stop stop-color="#2ee6f6" stop-opacity=".10"/><stop offset=".6" stop-color="#2ee6f6" stop-opacity=".03"/><stop offset="1" stop-color="#2ee6f6" stop-opacity="0"/></linearGradient>
</defs>""")
add(f'<rect width="{W}" height="{H}" fill="#05070b"/>')

def pane(x0, x1, top, seed_lines, minimap=True, number_from=1):
    """One editor pane between x0 and x1."""
    gutter = 58
    add(f'<rect x="{x0}" y="{top-30}" width="{x1-x0}" height="{H-top+30}" fill="#070a10" opacity=".9"/>')
    # tab strip
    add(f'<rect x="{x0}" y="{top-30}" width="{x1-x0}" height="30" fill="#0a0e15"/>')
    tx = x0 + 10
    for i, wtab in enumerate(rng.sample([96, 120, 84, 132, 108], 3)):
        add(f'<rect x="{tx}" y="{top-24}" width="{wtab}" height="20" rx="4" fill="{"#111a26" if i == 0 else "#0c121b"}"/>')
        add(f'<rect x="{tx+10}" y="{top-16}" width="{wtab-34}" height="4" rx="2" fill="#8b98a8" opacity="{.45 if i == 0 else .2}"/>')
        if i == 0:
            add(f'<rect x="{tx}" y="{top-6}" width="{wtab}" height="2" fill="#2ee6f6" opacity=".55"/>')
        tx += wtab + 6
    code_x = x0 + gutter + 14
    right = x1 - (110 if minimap else 30)
    rows = (H - top) // LINE
    indent = 0
    cur = rng.randint(5, 9)
    for r in range(rows):
        y = top + r * LINE
        n = number_from + r
        if r == cur:
            add(f'<rect x="{x0}" y="{y-14}" width="{x1-x0}" height="{LINE}" fill="url(#cur)"/>')
            add(f'<rect x="{x0}" y="{y-14}" width="2" height="{LINE}" fill="#2ee6f6" opacity=".7"/>')
        add(f'<text x="{x0+gutter-8}" y="{y}" text-anchor="end" font-family="JetBrains Mono, ui-monospace, monospace" font-size="11" fill="{"#2ee6f6" if r == cur else "#3a4656"}" opacity="{.8 if r == cur else .7}">{n}</text>')
        roll = rng.random()
        if roll < 0.13:  # blank line
            indent = max(0, indent - rng.choice([0, 1]))
            continue
        if roll < 0.22:  # comment
            wcm = rng.randint(160, 420)
            add(f'<rect x="{code_x + indent*28}" y="{y-8}" width="{min(wcm, right-code_x-indent*28)}" height="7" rx="3.5" fill="#5d6a7a" opacity=".32"/>')
            continue
        # indent guides
        for g in range(indent):
            add(f'<rect x="{code_x + g*28 + 2}" y="{y-14}" width="1" height="{LINE}" fill="#1c2633"/>')
        x = code_x + indent * 28
        for _ in range(rng.randint(2, 7)):
            wt = rng.choice([18, 24, 30, 36, 44, 56, 72, 90, 120])
            if x + wt > right:
                break
            color = rng.choices(COLORS, WEIGHTS)[0]
            op = rng.uniform(.28, .5) if color != "#8b98a8" else rng.uniform(.18, .3)
            add(f'<rect x="{x}" y="{y-8}" width="{wt}" height="7" rx="3.5" fill="{color}" opacity="{op:.2f}"/>')
            x += wt + rng.choice([8, 8, 10, 14])
        # block structure: open / close
        step = rng.random()
        if step < 0.24 and indent < 5:
            indent += 1
        elif step > 0.72 and indent > 0:
            indent -= 1
    if minimap:
        mx = x1 - 92
        add(f'<rect x="{mx-8}" y="{top-6}" width="94" height="{H-top+6}" fill="#060910" opacity=".8"/>')
        add(f'<rect x="{mx-8}" y="{top+40}" width="94" height="150" fill="#2ee6f6" opacity=".04"/>')
        for r in range(0, (H - top) // 4):
            y = top + r * 4
            xx = mx + rng.choice([0, 0, 6, 12, 18])
            for _ in range(rng.randint(0, 4)):
                wt = rng.randint(4, 22)
                if xx + wt > mx + 80:
                    break
                add(f'<rect x="{xx}" y="{y}" width="{wt}" height="2" fill="{rng.choices(COLORS, WEIGHTS)[0]}" opacity=".35"/>')
                xx += wt + 3

split = 1150
pane(0, split, 104, 0, minimap=True, number_from=1)
add(f'<rect x="{split}" y="74" width="2" height="{H}" fill="#121a26"/>')
pane(split + 2, W, 104, 0, minimap=True, number_from=118)
# title bar
add(f'<rect width="{W}" height="74" fill="#05070b"/>')
add(f'<rect y="73" width="{W}" height="1" fill="#141c28"/>')
# glows and fade on top
add(f'<rect width="{W}" height="{H}" fill="url(#gv)"/>')
add(f'<rect width="{W}" height="{H}" fill="url(#gc)"/>')
add(f'<rect width="{W}" height="{H}" fill="url(#gm)"/>')
add(f'<rect width="{W}" height="{H}" fill="url(#fade)"/>')
add("</svg>")

dest = Path(__file__).resolve().parent.parent / "site/assets/bg-editor.svg"
dest.write_text("\n".join(out) + "\n")
print(dest, round(dest.stat().st_size / 1024), "KiB")
