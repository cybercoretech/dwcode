#!/usr/bin/env python3
"""Snapshot canonical Cybercore palettes into DWCode's shell/editor token schema."""
import json, pathlib, shutil
root=pathlib.Path(__file__).resolve().parents[1]
upstream=root.parents[1]/'framework/cybercore'
out=root/'src/data';out.mkdir(exist_ok=True)
themes=[]
for file in sorted((upstream/'schema/themes').glob('*/*.json')):
 p=json.loads(file.read_text())
 if not all(k in p for k in ('bg','white','panel','line','cyan','hot_pink','muted')):continue
 c=lambda k:'#'+p[k].lstrip('#')
 themes.append(dict(id=file.stem,name=file.stem.replace('-',' ').title(),family=file.parent.name,appearance='dark',surfaces=dict(window=c('bg'),shell=c('panel'),sidebar=c('panel'),editor=c('bg'),raised=c('panel'),border=c('line')),text=dict(primary=c('white'),secondary=c('white'),muted=c('muted')),accents=dict(primary=c('cyan'),secondary=c('hot_pink'),success=c('acid_green'),warning=c('orange'),error=c('red')),syntax=dict(comment=c('muted'),keyword=c('hot_pink'),string=c('acid_green'),number=c('orange'),type=c('cyan'),function=c('purple'),variable=c('white'),constant=c('purple'))))
for file in sorted((root/'packs/dwcode-core/themes').glob('*.json')):
 p=json.loads(file.read_text());p['family']='DWCode';themes.append(p)
(out/'themes.json').write_text(json.dumps(themes,indent=2)+'\n')
shutil.copy2(upstream/'LICENSE-MIT',out/'CYBERCORE-LICENSE-MIT')
(out/'README.md').write_text('# Cybercore themes\n\nCanonical palettes from cybercoretech/cybercore, schema/themes. MIT license retained in CYBERCORE-LICENSE-MIT. DWCode token mapping: tools/sync-cybercore-themes.py. Local DWCode packs retain their existing license status.\n')
print(f'{len(themes)} palettes exported')
