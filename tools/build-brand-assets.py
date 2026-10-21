#!/usr/bin/env python3
"""Package approved Railshift artwork and its small-format vector companion."""
from pathlib import Path
import base64, subprocess, shutil, json, hashlib, zipfile
root=Path(__file__).resolve().parents[1];pack=root/'brand/railshift'
for folder in ['png','svg','icons','avatars','social']: (pack/folder).mkdir(exist_ok=True)
master=pack/'source/railshift-transparent.png'
def magick(*args):subprocess.run(['magick',*map(str,args)],check=True)
def svg(name,body,view='0 0 1200 680'):
 text=f'<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" viewBox="{view}" role="img"><title>DWCode Railshift</title>{body}</svg>\n';(pack/'svg'/name).write_text(text);return text
D='M24 36H360L510 178V518L360 648H24V486L68 434V214L24 166ZM195 213V503H304L360 449V268L304 213Z'
W='M480 36H600L686 313L796 126H866L968 313L1060 36H1180L1018 648H923L827 399L735 648H626Z'
defs='<defs><linearGradient id="steel" x1="0" y1="0" x2=".8" y2="1"><stop stop-color="#e2edf2"/><stop offset=".16" stop-color="#39434b"/><stop offset=".45" stop-color="#151c24"/><stop offset=".7" stop-color="#48535c"/><stop offset="1" stop-color="#161d26"/></linearGradient><linearGradient id="edge" x2=".8" y2="1"><stop stop-color="#f2fbff"/><stop offset=".25" stop-color="#7a8994"/><stop offset=".52" stop-color="#f1f8fb"/><stop offset="1" stop-color="#697987"/></linearGradient></defs>'
body=defs+f'<g stroke="url(#edge)" stroke-width="13" stroke-linejoin="bevel"><path d="{D}" fill="url(#steel)" fill-rule="evenodd"/><path d="{W}" fill="url(#steel)"/></g><g fill="none" stroke-linejoin="bevel" stroke-width="8"><path d="M57 65H342L471 190V497L341 611H59L153 517V202L107 153V85H149L182 116H316" stroke="#32dcff"/><path d="M526 62L656 614H709L827 302L948 614H988L1137 66M826 157V280" stroke="#ff33af"/></g><g fill="#090d15" stroke="#afbcc5" stroke-width="4"><circle cx="341" cy="85" r="12"/><circle cx="108" cy="166" r="10"/><circle cx="108" cy="537" r="10"/><circle cx="360" cy="569" r="12"/><circle cx="680" cy="410" r="10"/><circle cx="1004" cy="375" r="10"/></g><path d="M147 570L172 540M178 570L203 540M209 570L234 540" stroke="#32dcff" stroke-width="10"/><path d="M547 91L582 104M554 117L589 130M561 143L596 156" stroke="#10141b" stroke-width="10"/>'
vector=svg('railshift-vector.svg',body)
svg('railshift-mono.svg',f'<g fill="#e5edf5"><path d="{D}" fill-rule="evenodd"/><path d="{W}"/></g>')
small=f'<rect width="256" height="256" rx="42" fill="#090c13"/><g transform="translate(15 63) scale(.188)"><path d="{D}" fill="#44dcff" fill-rule="evenodd"/><path d="{W}" fill="#ff42b5"/></g>'
svg('railshift-favicon.svg',small,'0 0 256 256')
embedded='data:image/png;base64,'+base64.b64encode(master.read_bytes()).decode()
svg('railshift-artwork.svg',f'<desc>Approved detailed artwork embedded as PNG. Use railshift-vector.svg for editable vector paths.</desc><image width="1254" height="1254" xlink:href="{embedded}"/>','0 0 1254 1254')
for n in [64,128,256,512,1024]:
 magick(master,'-resize',f'{n}x{n}',pack/'png'/f'railshift-transparent-{n}.png')
 magick(master,'-trim','+repage','-bordercolor','none','-border','20','-resize',f'{n}x',pack/'png'/f'railshift-wide-{n}.png')
for n in [64,128,256,512,1024]:
 magick(master,'-background','#080b12','-alpha','remove','-alpha','off','-resize',f'{n}x{n}',pack/'avatars'/f'railshift-avatar-{n}.png')
for n in [16,24,32,48,64,128,256]:magick('-background','none',pack/'svg/railshift-favicon.svg','-resize',f'{n}x{n}',pack/'icons'/f'favicon-{n}.png')
magick(*[pack/'icons'/f'favicon-{n}.png' for n in [16,24,32,48,64,128,256]],pack/'icons/favicon.ico')
for n,name in [(180,'apple-touch-icon.png'),(192,'android-chrome-192.png'),(512,'android-chrome-512.png')]:magick('-background','none',pack/'svg/railshift-favicon.svg','-resize',f'{n}x{n}',pack/'icons'/name)
# Social artwork uses the approved detailed mark, composed with editable SVG text.
social=f'<rect width="1200" height="630" fill="#080b12"/><path d="M60 88H1140M60 540H1140" stroke="#253441"/><path d="M60 88H260" stroke="#43dcff" stroke-width="3"/><path d="M940 540H1140" stroke="#ff42b5" stroke-width="3"/><text x="64" y="211" fill="#43dcff" font-family="DejaVu Sans,sans-serif" font-size="17" letter-spacing="5">CYBERCORE / DWCODE</text><text x="60" y="300" fill="#edf4fc" font-family="DejaVu Sans,sans-serif" font-size="68" font-weight="bold">Code after dark.</text><text x="64" y="365" fill="#aab8c6" font-family="DejaVu Sans,sans-serif" font-size="24">A workspace tuned to your signal.</text><text x="64" y="487" fill="#7e8c9b" font-family="DejaVu Sans,sans-serif" font-size="15" letter-spacing="3">DESKTOP EDITOR / RAILSHIFT IDENTITY</text><image x="650" y="65" width="510" height="510" xlink:href="{embedded}"/>'
s=svg('railshift-social.svg',social,'0 0 1200 630');magick('-background','none',pack/'svg/railshift-social.svg','-resize','1200x630',pack/'social/dwcode-social-1200x630.png')
manifest={'name':'DWCode','short_name':'DWCode','start_url':'./','display':'standalone','background_color':'#080b12','theme_color':'#080b12','icons':[{'src':'assets/android-chrome-192.png','sizes':'192x192','type':'image/png'},{'src':'assets/android-chrome-512.png','sizes':'512x512','type':'image/png'}]}
(root/'site/site.webmanifest').write_text(json.dumps(manifest,indent=2)+'\n')
(pack/'README.md').write_text('''# DWCode — Railshift asset pack

Selected by the project owner on 2026-10-21.

- `source/railshift-approved.png`: untouched approved concept, 1254×1254.
- `source/railshift-transparent.png`: background removal via the built-in image generator, preserving the approved design.
- `png/railshift-transparent-*`: transparent square PNGs, 64–1024 px.
- `png/railshift-wide-*`: trimmed horizontal transparent PNGs, widths 64–1024 px.
- `avatars/`: dark square profiles, 64, 128, 256, 512 and 1024 px.
- `svg/railshift-artwork.svg`: the detailed PNG embedded inside SVG; not vector-traced.
- `svg/railshift-vector.svg`: editable, simplified vector companion, with metallic rails and inset cyan-magenta channels.
- `svg/railshift-mono.svg`: monochrome vector companion.
- `svg/railshift-favicon.svg`: simplified high-contrast icon on a dark square.
- `icons/favicon.ico`: multi-resolution 16, 24, 32, 48, 64, 128, 256 px.
- `icons/`: PNG favicons, Apple touch icon (180), Android icons (192, 512).
- `social/`: 1200×630 social card, editable composition in `svg/railshift-social.svg`.

Use the detailed approved artwork for the website hero and avatars. Use the simplified vector/icon at small sizes. Keep cyan on the D and magenta on the W; keep steel highlights neutral. Give the mark at least 8% clear space. The simplified vectors are companions, not exact tracings of the detailed artwork. No app screenshot is AI-generated.

Rebuild with `python3 tools/build-brand-assets.py` from the repository root. ImageMagick performs deterministic resizing and format conversion. Source artwork uses the built-in image generator; the background-removal prompt is in SOURCE.md. All rights remain with the project owner; no new redistribution license is granted by this pack.
''')
(pack/'SOURCE.md').write_text('''# Source and generation

Approved source: brand/fusion-concepts/04-railshift.png; prompt in ../fusion-concepts/PROMPTS.md.

Built-in image generator edit prompt: Remove only the black background, including the D counter and W gaps. Preserve the approved Railshift geometry, steel surfaces, cyan and magenta lighting, fasteners and vents. Keep black metal opaque. Genuine transparent alpha, narrow neon halo, square canvas. No redesign.

Simplified companion SVG paths and social layout are hand-authored code-native assets. Raster exports are derived using ImageMagick.
''')
files=sorted(p for p in pack.rglob('*') if p.is_file() and p.name!='SHA256SUMS')
(pack/'SHA256SUMS').write_text('\n'.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+str(p.relative_to(pack)) for p in files)+'\n')
archive=root/'brand/dwcode-railshift-assets.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
 for p in sorted(pack.rglob('*')):
  if p.is_file():z.write(p,'dwcode-railshift/'+str(p.relative_to(pack)))
assets=root/'site/assets';assets.mkdir(exist_ok=True)
for src,dst in [(pack/'png/railshift-wide-1024.png','railshift.png'),(pack/'svg/railshift-vector.svg','railshift.svg'),(pack/'svg/railshift-favicon.svg','favicon.svg'),(pack/'icons/favicon.ico','favicon.ico'),(pack/'icons/apple-touch-icon.png','apple-touch-icon.png'),(pack/'icons/android-chrome-192.png','android-chrome-192.png'),(pack/'icons/android-chrome-512.png','android-chrome-512.png'),(pack/'social/dwcode-social-1200x630.png','social.png')]:shutil.copy2(src,assets/dst)
shutil.copy2(archive,root/'site/downloads/dwcode-railshift-assets.zip')
print(f'{len(files)+1} files; archive: {archive}')
