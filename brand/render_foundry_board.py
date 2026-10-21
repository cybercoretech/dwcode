"""Render the DWCode logo-option contact board with Blender's Eevee renderer."""

import bpy

OUT = "/home/raven/Devspace/Cybercore/tooling/dwcode/brand/dwcode-logo-board-base.png"
NAMES = ["CORELINE DW", "SPLIT REACTOR", "DATAFORGE", "PHASE DRIVER", "WORKSPACE PRISM", "COMMAND SURGE", "DARKWAVE RELAY", "KERNEL APERTURE", "SYSTEM LATTICE", "DW VECTOR"]
ACCENTS = [(0.01, .95, .82), (.1, .7, 1), (0, .95, .78), (.9, .05, .6), (.1, .8, .95), (.85, .1, .58), (0, .9, .77), (.45, .25, 1), (.05, .85, .78), (.9, .12, .65)]

bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
scene = bpy.context.scene
scene.render.engine = "BLENDER_EEVEE"
scene.eevee.taa_render_samples = 48
scene.render.resolution_x = 2400
scene.render.resolution_y = 1600
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = "PNG"
scene.render.image_settings.color_mode = "RGBA"
scene.view_settings.view_transform = "AgX"
scene.world.color = (.0015, .003, .006)
scene.render.filepath = OUT

def mat(name, color, metallic=0, roughness=.4, emission=0):
    m = bpy.data.materials.new(name)
    m.diffuse_color = (*color, 1)
    m.use_nodes = True
    s = m.node_tree.nodes.get("Principled BSDF")
    s.inputs["Base Color"].default_value = (*color, 1)
    s.inputs["Metallic"].default_value = metallic
    s.inputs["Roughness"].default_value = roughness
    if emission:
        s.inputs["Emission Color"].default_value = (*color, 1)
        s.inputs["Emission Strength"].default_value = emission
    return m

def emissive(name, color, strength=2):
    m = bpy.data.materials.new(name)
    m.diffuse_color = (*color, 1)
    m.use_nodes = True
    nodes = m.node_tree.nodes
    nodes.clear()
    out = nodes.new("ShaderNodeOutputMaterial")
    emission = nodes.new("ShaderNodeEmission")
    emission.inputs["Color"].default_value = (*color, 1)
    emission.inputs["Strength"].default_value = strength
    m.node_tree.links.new(emission.outputs["Emission"], out.inputs["Surface"])
    return m

def cube(name, loc, dims, material, bevel=.04):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = dims
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    obj.data.materials.append(material)
    if bevel:
        mod = obj.modifiers.new("CNC edge radius", "BEVEL")
        mod.width = bevel
        mod.segments = 4
        obj.modifiers.new("Machined normals", "WEIGHTED_NORMAL")
    return obj

def text(body, loc, size, material, align="LEFT"):
    curve = bpy.data.curves.new("Glyph / " + body, "FONT")
    curve.body = body
    curve.size = size
    curve.align_x = align
    curve.space_character = 1.15
    obj = bpy.data.objects.new(body, curve)
    scene.collection.objects.link(obj)
    obj.location = loc
    obj.data.materials.append(material)
    return obj

def area(name, loc, color, power, size):
    data = bpy.data.lights.new(name, "AREA")
    data.energy = power
    data.color = color
    data.shape = "DISK"
    data.size = size
    obj = bpy.data.objects.new(name, data)
    scene.collection.objects.link(obj)
    obj.location = loc
    return obj

obsidian = mat("Obsidian substrate", (.005, .009, .015), .28, .52)
titanium = mat("Anodized titanium", (.018, .028, .04), .82, .28)
glass = mat("Inset smoked glass", (.007, .015, .024), .58, .22)
rails = mat("Graphite datum rails", (.032, .05, .064), .75, .32)
white = mat("Cool white lettering", (.68, .84, .87), .1, .42, .12)
muted = mat("Telemetry lettering", (.18, .3, .34), .1, .48, .05)

cube("Foundry backplane", (0, 0, -.48), (28.2, 18.7, .72), obsidian, .18)
for x in (-13.45, 13.45):
    cube("Vertical datum", (x, 0, -.06), (.025, 16.4, .025), rails, .01)
for y in (7.55, -7.55):
    cube("Horizontal datum", (0, y, -.06), (26.9, .025, .025), rails, .01)
text("DWCODE  /  IDENTITY FOUNDRY", (-12.85, 8, 0), .38, white)
text("TEN ORIGINAL SYSTEM MARKS   /   EDITORIAL SERIES 01", (-12.82, 7.55, 0), .13, muted)
text("DWCODE  ·  DESIGN STUDY  ·  2026", (12.8, 8, 0), .15, muted, "RIGHT")

xs, ys = [-10.7, -5.35, 0, 5.35, 10.7], [3.25, -3.95]
for i, name in enumerate(NAMES):
    row, col = divmod(i, 5)
    x, y = xs[col], ys[row]
    signal = emissive("Signal %02d" % (i + 1), ACCENTS[i], 2.0)
    cube("P%02d / titanium shell" % (i + 1), (x, y, -.03), (4.92, 6.62, .42), titanium, .12)
    cube("P%02d / recessed glass" % (i + 1), (x, y + .08, .205), (4.5, 5.65, .035), glass, .055)
    cube("P%02d / status rail" % (i + 1), (x + 1.72, y + 2.92, .24), (.78, .055, .035), signal, .025)
    cube("P%02d / index lamp" % (i + 1), (x - 1.85, y + 2.91, .24), (.12, .12, .04), signal, .04)
    cube("P%02d / inset datum" % (i + 1), (x, y - 1.72, .24), (3.7, .018, .018), rails, .008)
    text("SYS  /  %02d" % (i + 1), (x, y - 2.18, .24), .15, signal, "CENTER")
    text(name, (x, y - 2.7, .24), .205, white, "CENTER")
    text("DWCODE  //  SIGNAL CORE", (x, y - 3.05, .24), .095, muted, "CENTER")

area("Cyan softbox", (-8, 5, 10), (.06, .75, .86), 1150, 9)
area("Magenta softbox", (9, -3, 9), (.75, .06, .47), 850, 8)
area("Cool top fill", (1, 10, 13), (.56, .76, .9), 1250, 11)
camera_data = bpy.data.cameras.new("Foundry camera")
camera = bpy.data.objects.new("Foundry camera", camera_data)
scene.collection.objects.link(camera)
camera.location = (0, 0, 26)
camera_data.type = "ORTHO"
camera_data.ortho_scale = 28
scene.camera = camera

bpy.ops.render.render(write_still=True)
