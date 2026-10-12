import math
import sys
from pathlib import Path

import bpy
from mathutils import Vector


ROOT = Path(__file__).resolve().parents[1]
ADDON_ROOT = ROOT / ".tmp" / "UEFormat" / "plugins" / "blender"
SOURCE_ROOT = (
    ROOT
    / "dist"
    / "plant-satchel-render-source"
    / "RSDragonwilds"
    / "Content"
    / "Art"
    / "Item"
    / "Resources"
    / "Plant_Satchel"
)
MODEL = SOURCE_ROOT / "SM_Plant_Satchel_01.uemodel"
BASE_COLOR = SOURCE_ROOT / "Textures" / "T_Plant_Satchel_01_BCC.png"
NORMAL = SOURCE_ROOT / "Textures" / "T_Plant_Satchel_01_NRM.png"
OUTPUT = ROOT / "web" / "items" / "SM_Plant_Satchel_01.png"


sys.path.insert(0, str(ADDON_ROOT))
from io_scene_ueformat.importer.import_context import UEFormatImport  # noqa: E402
from io_scene_ueformat.options import UEModelOptions  # noqa: E402


bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
UEFormatImport(UEModelOptions(scale_factor=0.01, import_collision=False)).import_file(MODEL)

meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
if not meshes:
    raise RuntimeError("The satchel model did not produce a mesh")

material = bpy.data.materials.new("Plant Satchel")
material.use_nodes = True
nodes = material.node_tree.nodes
links = material.node_tree.links
principled = nodes.get("Principled BSDF")

color = nodes.new("ShaderNodeTexImage")
color.image = bpy.data.images.load(str(BASE_COLOR))
color.image.colorspace_settings.name = "sRGB"
links.new(color.outputs["Color"], principled.inputs["Base Color"])
links.new(color.outputs["Alpha"], principled.inputs["Alpha"])

normal_image = nodes.new("ShaderNodeTexImage")
normal_image.image = bpy.data.images.load(str(NORMAL))
normal_image.image.colorspace_settings.name = "Non-Color"
normal_map = nodes.new("ShaderNodeNormalMap")
normal_map.inputs["Strength"].default_value = 0.75
links.new(normal_image.outputs["Color"], normal_map.inputs["Color"])
links.new(normal_map.outputs["Normal"], principled.inputs["Normal"])
principled.inputs["Roughness"].default_value = 0.72

for obj in meshes:
    obj.data.materials.clear()
    obj.data.materials.append(material)

world_corners = []
for obj in meshes:
    world_corners.extend(obj.matrix_world @ Vector(corner) for corner in obj.bound_box)
center = sum(world_corners, Vector()) / len(world_corners)
for obj in meshes:
    obj.location -= center

# Match the familiar dropped-item view: front face visible, handle rising behind it.
for obj in meshes:
    obj.rotation_euler.rotate_axis("Z", math.radians(-8))

world_corners = []
for obj in meshes:
    world_corners.extend(obj.matrix_world @ Vector(corner) for corner in obj.bound_box)
extent = max(max(abs(v.x), abs(v.y), abs(v.z)) for v in world_corners)

camera_data = bpy.data.cameras.new("Camera")
camera = bpy.data.objects.new("Camera", camera_data)
bpy.context.collection.objects.link(camera)
bpy.context.scene.camera = camera
camera.location = Vector((extent * 3.8, -extent * 5.4, extent * 2.5))
direction = Vector((0, 0, extent * 0.05)) - camera.location
camera.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
camera.data.type = "ORTHO"
camera.data.ortho_scale = extent * 2.35

key_data = bpy.data.lights.new("Key", type="AREA")
key_data.energy = 950
key_data.shape = "DISK"
key_data.size = extent * 3.0
key = bpy.data.objects.new("Key", key_data)
key.location = Vector((-extent * 2.0, -extent * 3.0, extent * 5.0))
bpy.context.collection.objects.link(key)

fill_data = bpy.data.lights.new("Fill", type="AREA")
fill_data.energy = 550
fill_data.size = extent * 2.5
fill = bpy.data.objects.new("Fill", fill_data)
fill.location = Vector((extent * 4.0, extent * 1.0, extent * 2.5))
bpy.context.collection.objects.link(fill)

scene = bpy.context.scene
scene.render.engine = "BLENDER_EEVEE"
scene.render.resolution_x = 256
scene.render.resolution_y = 256
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = "PNG"
scene.render.image_settings.color_mode = "RGBA"
scene.render.film_transparent = True
scene.render.filepath = str(OUTPUT)
scene.render.image_settings.color_depth = "8"
scene.view_settings.look = "AgX - Medium High Contrast"
scene.world.color = (0.025, 0.035, 0.03)
scene.render.image_settings.compression = 9

OUTPUT.parent.mkdir(parents=True, exist_ok=True)
bpy.ops.render.render(write_still=True)
