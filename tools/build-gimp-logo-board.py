"""GIMP 3 batch script: make an editable XCF from the Blender board and SVG marks.

Run with:
  gimp-console -n -i -d -f --batch-interpreter=python-fu-eval \
    -b 'exec(open("tools/build-gimp-logo-board.py").read())' --quit
"""

import glob
import os

root = "/home/raven/Devspace/Cybercore/tooling/dwcode"
brand = os.path.join(root, "brand")
image = Gimp.file_load(
    Gimp.RunMode.NONINTERACTIVE,
    Gio.File.new_for_path(os.path.join(brand, "dwcode-logo-board-base.png")),
)

xs = [-10.7, -5.35, 0, 5.35, 10.7]
ys = [3.25, -3.95]
for index, path in enumerate(sorted(glob.glob(os.path.join(brand, "options", "*.png")))):
    row, col = divmod(index, 5)
    x, y = xs[col], ys[row] + 0.7
    center_x = round((x + 14) / 28 * 2400)
    center_y = round((9.333333 - y) / 18.666667 * 1600)
    layer = Gimp.file_load_layer(Gimp.RunMode.NONINTERACTIVE, image, Gio.File.new_for_path(path))
    layer.set_name("%02d / %s" % (index + 1, os.path.basename(path).removesuffix(".png")))
    image.insert_layer(layer, None, 0)
    layer.set_offsets(center_x - 150, center_y - 150)

target = Gio.File.new_for_path(os.path.join(brand, "dwcode-logo-options.xcf"))
if not Gimp.file_save(Gimp.RunMode.NONINTERACTIVE, image, target):
    raise RuntimeError("GIMP could not save the editable logo board")
print("Saved editable XCF with %d independent logo layers" % len(image.get_layers()))
