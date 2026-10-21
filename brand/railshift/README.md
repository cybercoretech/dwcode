# DWCode — Railshift asset pack

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
