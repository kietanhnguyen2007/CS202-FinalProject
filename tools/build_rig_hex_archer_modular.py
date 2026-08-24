"""Procedurally build the production Hex Archer body + rigid bow pair.

The body and bow are reconstructed locally as clean low-poly geometry from two
separate concept turnarounds.  The body is skinned to the same 16-joint runtime contract as the modular heroes,
including a non-weighted ``weapon_socket.R`` joint.  The bow is normalized as a
rigid, animation-free GLB whose grip is at local origin.  QA renders temporarily
attach that bow to the animated socket, but it is never exported inside the body.

Run from the repository root with Blender 4.5 LTS::

    blender -b --python tools/build_rig_hex_archer_modular.py

Concept references::

    assets/survival3d/concepts/characters/modular/hex_archer/hex_archer_body_turnaround_v1.png
    assets/survival3d/concepts/characters/modular/hex_archer/hex_archer_bow_turnaround_v1.png

Use ``--no-render`` for a structural smoke test and ``--strict-missing`` when a
missing concept must fail CI instead of producing a pending report.
"""

import argparse
import importlib.util
import json
import math
import os
import statistics
import sys
import traceback

import bpy
from mathutils import Vector


ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
PRODUCTION = os.path.join(ROOT, "assets", "survival3d", "production_v3")
DEFAULT_OUTPUT_ROOT = os.path.join(PRODUCTION, "step5_staging")
CONCEPT_ROOT = os.path.join(
    ROOT, "assets", "survival3d", "concepts", "characters", "modular", "hex_archer"
)
DEFAULT_BODY_CONCEPT = os.path.join(CONCEPT_ROOT, "hex_archer_body_turnaround_v1.png")
DEFAULT_BOW_CONCEPT = os.path.join(CONCEPT_ROOT, "hex_archer_bow_turnaround_v1.png")

BODY_OUTPUT_NAME = "hex_archer_animated.glb"
BOW_OUTPUT_NAME = "hex_archer_bow.glb"
BLEND_OUTPUT_NAME = "hex_archer_modular_rig.blend"
REPORT_NAME = "hex_archer_modular_report.json"

ACTOR = {
    "id": "hex_archer",
    # The caster branch has the topology-aware cape/hood smoothing needed by
    # the Archer silhouette.  Combat poses are replaced below with archery.
    "style": "caster",
    "object": "HexArcherBody",
    "body_height": 2.05,
}

QA_POSES = [
    (0, "equipped_rest", False, False),
    (104, "equipped_run", True, False),
    (184, "basic_draw", True, False),
    (202, "basic_release", True, False),
    (260, "skill_one_draw", True, False),
    (274, "skill_one_release", True, False),
    (344, "skill_two_release", True, False),
    (432, "ultimate_draw", True, False),
    (448, "ultimate_release", True, False),
    (505, "dash", True, False),
    (540, "hurt", True, False),
    (638, "death", True, False),
    (184, "basic_draw_rear", True, True),
]

COMBAT_EVENTS = [
    {"clip": "basic", "frame": 202, "event": "ReleaseArrow"},
    {"clip": "skillOne", "frame": 260, "event": "ReleaseArrowLeft"},
    {"clip": "skillOne", "frame": 274, "event": "ReleaseArrowCenter"},
    {"clip": "skillOne", "frame": 286, "event": "ReleaseArrowRight"},
    {"clip": "skillTwo", "frame": 344, "event": "ReleasePiercingArrow"},
    {"clip": "ultimatePhase", "frame": 448, "event": "ReleaseHexVolley"},
]


def load_modular_pipeline():
    path = os.path.join(ROOT, "tools", "rig_animate_modular_heroes.py")
    spec = importlib.util.spec_from_file_location("modular_hero_pipeline", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Cannot load modular hero pipeline: {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    # Extend its weapon leak detector for the Archer contract.
    module.WEAPON_WORDS = tuple(
        dict.fromkeys((*module.WEAPON_WORDS, "bow", "arrow", "quiver"))
    )
    return module


BASE = load_modular_pipeline()


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--body-concept", default=DEFAULT_BODY_CONCEPT)
    parser.add_argument("--bow-concept", default=DEFAULT_BOW_CONCEPT)
    parser.add_argument("--output-root", default=DEFAULT_OUTPUT_ROOT)
    parser.add_argument("--no-render", action="store_true")
    parser.add_argument("--strict-missing", action="store_true")
    values = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    args = parser.parse_args(values)
    args.body_concept = absolute_path(args.body_concept)
    args.bow_concept = absolute_path(args.bow_concept)
    args.output_root = absolute_path(args.output_root)
    return args


def absolute_path(path):
    return path if os.path.isabs(path) else os.path.abspath(os.path.join(ROOT, path))


def relative(path):
    return os.path.relpath(path, ROOT).replace("\\", "/")


def output_paths(output_root):
    model_dir = os.path.join(output_root, "models", "enemies")
    work_dir = os.path.join(output_root, "work")
    return {
        "body": os.path.join(model_dir, BODY_OUTPUT_NAME),
        "bow": os.path.join(model_dir, BOW_OUTPUT_NAME),
        "blend": os.path.join(work_dir, BLEND_OUTPUT_NAME),
        "report": os.path.join(output_root, REPORT_NAME),
        "qa": os.path.join(output_root, "qa", "animation"),
    }


def write_report(path, report):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    temporary = path + ".tmp"
    with open(temporary, "w", encoding="utf-8") as handle:
        json.dump(report, handle, indent=2)
        handle.write("\n")
    os.replace(temporary, path)


def bounds_for_object(mesh):
    points = [mesh.matrix_world @ vertex.co for vertex in mesh.data.vertices]
    if not points:
        raise RuntimeError(f"Mesh {mesh.name!r} has no vertices")
    return (
        min(point.x for point in points), max(point.x for point in points),
        min(point.y for point in points), max(point.y for point in points),
        min(point.z for point in points), max(point.z for point in points),
    )


def audit_archer_body_only_geometry(mesh, bounds):
    """Accept layered clothes while rejecting a bow-expanded body silhouette.

    Hunyuan commonly emits the hood, cloak, gloves and boots as disconnected
    islands inside one mesh object.  Component count therefore cannot prove a
    weapon leak.  A fused or forgotten longbow does, however, expand the body
    envelope strongly in width/depth; combine that envelope contract with the
    object/material name checks performed by ``validate_builder_surface``.
    """
    width = bounds[1] - bounds[0]
    depth = bounds[3] - bounds[2]
    height = bounds[5] - bounds[4]
    components = BASE.connected_component_sizes(mesh)
    major_threshold = max(32, int(len(mesh.data.vertices) * .02))
    major_components = [size for size in components if size >= major_threshold]
    if width / height > .76 or depth / height > .62:
        raise RuntimeError(
            "Hex Archer body envelope suggests embedded bow/arrow geometry: "
            f"width/height={width / height:.4f}, depth/height={depth / height:.4f}"
        )
    if not mesh.get("procedural_rebuild", False) and len(major_components) > 16:
        raise RuntimeError(
            "Hex Archer body has too many major disconnected islands for a clean "
            f"character source: {major_components}"
        )
    return {
        "componentCount": len(components),
        "majorComponentCount": len(major_components),
        "majorComponentVertices": major_components,
        "proceduralLayeredIslandsAllowed": bool(mesh.get("procedural_rebuild", False)),
        "largestComponentVertices": components[0] if components else 0,
        "widthToHeight": round(width / height, 5),
        "depthToHeight": round(depth / height, 5),
        "weaponEnvelopeRejectedAbove": {"widthToHeight": .76, "depthToHeight": .62},
    }


def material(name, color, metallic=0.0, roughness=.72, emission=None):
    result = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    result.use_nodes = True
    result.diffuse_color = (*color, 1.0)
    shader = result.node_tree.nodes.get("Principled BSDF")
    if shader is not None:
        shader.inputs["Base Color"].default_value = (*color, 1.0)
        shader.inputs["Metallic"].default_value = metallic
        shader.inputs["Roughness"].default_value = roughness
        if emission is not None:
            if "Emission Color" in shader.inputs:
                shader.inputs["Emission Color"].default_value = (*emission, 1.0)
            elif "Emission" in shader.inputs:
                shader.inputs["Emission"].default_value = (*emission, 1.0)
            if "Emission Strength" in shader.inputs:
                shader.inputs["Emission Strength"].default_value = 1.6
    return result


def archer_materials():
    return {
        "cloth": material("HexArcher_Cloth", (.004, .020, .009), roughness=.88),
        "cloth_light": material("HexArcher_ClothLight", (.008, .040, .018), roughness=.82),
        "armor": material("HexArcher_Armor", (.007, .034, .021), metallic=.12, roughness=.58),
        "graphite": material("HexArcher_Graphite", (.004, .007, .006), metallic=.72, roughness=.32),
        "trim": material("HexArcher_Trim", (.070, .090, .082), metallic=.78, roughness=.28),
        "void": material("HexArcher_VoidFace", (.0002, .0006, .0003), roughness=.95),
        "emerald": material(
            "HexArcher_Emerald", (.015, .55, .002), metallic=.05, roughness=.20,
            emission=(.010, .68, .001),
        ),
        "wrap": material("HexArcher_BowWrap", (.115, .080, .045), roughness=.94),
        "string": material("HexArcher_BowString", (.025, .032, .030), metallic=.20, roughness=.55),
    }


def assign_material(obj, value):
    obj.data.materials.append(value)
    for polygon in obj.data.polygons:
        polygon.material_index = 0
        polygon.use_smooth = False


def assign_weights(obj, weights):
    indices = list(range(len(obj.data.vertices)))
    total = sum(weights.values())
    if not indices or total <= 0:
        raise RuntimeError(f"Invalid procedural component weights for {obj.name}")
    for name, value in weights.items():
        group = obj.vertex_groups.get(name) or obj.vertex_groups.new(name=name)
        group.add(indices, value / total, "REPLACE")


def finish_component(obj, name, value, weights=None):
    obj.name = name
    obj.data.name = name + "Mesh"
    BASE.select_only([obj], active=obj)
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    assign_material(obj, value)
    if weights:
        assign_weights(obj, weights)
    return obj


def add_ico(name, location, scale, value, weights=None, subdivisions=1):
    bpy.ops.mesh.primitive_ico_sphere_add(
        subdivisions=subdivisions,
        radius=1.0,
        location=location,
    )
    obj = bpy.context.object
    obj.scale = scale
    return finish_component(obj, name, value, weights)


def add_vertical_cone(
    name, z_bottom, z_top, radius_bottom, radius_top, depth_scale,
    value, weights=None, x=0.0, y=0.0, vertices=8,
):
    bpy.ops.mesh.primitive_cone_add(
        vertices=vertices,
        radius1=radius_bottom,
        radius2=radius_top,
        depth=z_top - z_bottom,
        end_fill_type="NGON",
        location=(x, y, (z_bottom + z_top) * .5),
    )
    obj = bpy.context.object
    obj.scale.y = depth_scale
    return finish_component(obj, name, value, weights)


def add_segment(name, start, end, radius, value, weights=None, radius_end=None, vertices=8):
    start, end = Vector(start), Vector(end)
    delta = end - start
    if delta.length <= 1e-6:
        raise RuntimeError(f"Degenerate segment {name}")
    bpy.ops.mesh.primitive_cone_add(
        vertices=vertices,
        radius1=radius,
        radius2=radius if radius_end is None else radius_end,
        depth=delta.length,
        end_fill_type="NGON",
        location=(start + end) * .5,
    )
    obj = bpy.context.object
    obj.rotation_mode = "QUATERNION"
    obj.rotation_quaternion = Vector((0, 0, 1)).rotation_difference(delta.normalized())
    return finish_component(obj, name, value, weights)


def add_box(name, location, scale, value, weights=None, rotation=(0, 0, 0)):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=location, rotation=rotation)
    obj = bpy.context.object
    obj.scale = scale
    return finish_component(obj, name, value, weights)


def add_extruded_polygon(name, xz_points, y, depth, value, weights=None):
    count = len(xz_points)
    vertices = [
        (x, y - depth * .5, z) for x, z in xz_points
    ] + [
        (x, y + depth * .5, z) for x, z in xz_points
    ]
    faces = [tuple(reversed(range(count))), tuple(range(count, count * 2))]
    for index in range(count):
        following = (index + 1) % count
        faces.append((index, following, count + following, count + index))
    mesh_data = bpy.data.meshes.new(name + "Mesh")
    mesh_data.from_pydata(vertices, [], faces)
    mesh_data.validate(clean_customdata=False)
    mesh_data.update()
    obj = bpy.data.objects.new(name, mesh_data)
    bpy.context.scene.collection.objects.link(obj)
    return finish_component(obj, name, value, weights)


def add_gem(name, location, scale, value, weights=None, rotation=(0, 0, 0)):
    obj = add_ico(name, location, scale, value, weights, subdivisions=1)
    obj.rotation_euler = rotation
    BASE.select_only([obj], active=obj)
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
    return obj


def join_components(objects, name):
    if not objects:
        raise RuntimeError(f"No procedural components for {name}")
    BASE.select_only(objects, active=objects[0])
    bpy.ops.object.join()
    result = bpy.context.view_layer.objects.active
    result.name = name
    result.data.name = name + "Mesh"
    for polygon in result.data.polygons:
        polygon.use_smooth = False
    while result.data.uv_layers:
        result.data.uv_layers.remove(result.data.uv_layers[0])
    result.data.uv_layers.new(name="UVMap")
    result.data.validate(clean_customdata=False)
    result.data.update()
    return result


def build_procedural_body(concept_path):
    """Recreate the hooded, faceless green Archer shown in the turnaround."""
    bpy.ops.wm.read_factory_settings(use_empty=True)
    BASE.configure_scene()
    mats = archer_materials()
    pieces = []

    # Feet, segmented boots, legs and protected knees.
    for side, sign in (("L", -1), ("R", 1)):
        x = sign * .135
        pieces.append(add_ico(
            f"Boot.{side}", (x, -.055, .115), (.125, .205, .115),
            mats["graphite"], {f"shin.{side}": 1.0},
        ))
        pieces.append(add_segment(
            f"ShinArmor.{side}", (x, 0, .22), (x, 0, .59), .088,
            mats["armor"], {f"shin.{side}": 1.0}, radius_end=.105,
        ))
        pieces.append(add_gem(
            f"KneePlate.{side}", (x, -.092, .605), (.115, .045, .105),
            mats["graphite"], {f"thigh.{side}": .35, f"shin.{side}": .65},
        ))
        pieces.append(add_segment(
            f"Thigh.{side}", (x, 0, .58), (x * .82, 0, .965), .108,
            mats["cloth_light"], {f"thigh.{side}": 1.0}, radius_end=.125,
        ))

    # Pelvis and the tapered armored torso.
    pieces.append(add_vertical_cone(
        "Pelvis", .88, 1.08, .255, .245, .68, mats["cloth"],
        {"hips": .88, "spine": .12}, vertices=10,
    ))
    pieces.append(add_vertical_cone(
        "Torso", 1.02, 1.50, .245, .345, .63, mats["armor"],
        {"spine": .45, "chest": .55}, vertices=10,
    ))
    pieces.append(add_vertical_cone(
        "Collar", 1.43, 1.57, .35, .255, .66, mats["graphite"],
        {"chest": .78, "head": .22}, vertices=10,
    ))

    # Angular chest layers reproduce the concept's overlapping leaf armor.
    pieces.append(add_extruded_polygon(
        "ChestPlate.Center",
        [(-.12, 1.48), (0, 1.36), (.12, 1.48), (0, 1.20)],
        -.225, .045, mats["trim"], {"chest": .72, "spine": .28},
    ))
    for side, sign in (("L", -1), ("R", 1)):
        pieces.append(add_extruded_polygon(
            f"ChestLeaf.{side}",
            [(0, 1.45), (sign * .29, 1.46), (sign * .22, 1.24), (0, 1.18)],
            -.205, .050, mats["cloth_light"], {"chest": .78, "spine": .22},
        ))

    # Belt, buckle and the bright emerald milestone gem.
    pieces.append(add_vertical_cone(
        "Belt", .96, 1.06, .285, .285, .68, mats["graphite"], {"hips": 1.0},
        vertices=12,
    ))
    pieces.append(add_gem(
        "BeltFrame", (0, -.207, 1.005), (.15, .048, .15),
        mats["trim"], {"hips": 1.0},
    ))
    pieces.append(add_gem(
        "BeltEmerald", (0, -.262, 1.005), (.072, .026, .085),
        mats["emerald"], {"hips": 1.0},
    ))

    # Layered front/back robe panels.  Each island has deterministic weights,
    # preventing a cape vertex from ever jumping onto an arm during a draw.
    front_panels = [
        ("L", [(-.27, .98), (-.03, .98), (-.05, .34), (-.20, .20), (-.30, .42)]),
        ("R", [(.03, .98), (.27, .98), (.30, .42), (.20, .20), (.05, .34)]),
    ]
    for side, polygon in front_panels:
        pieces.append(add_extruded_polygon(
            f"FrontRobe.{side}", polygon, -.155, .035, mats["cloth"],
            {"hips": .68, f"thigh.{side}": .32},
        ))
    for side, sign in (("L", -1), ("R", 1)):
        pieces.append(add_extruded_polygon(
            f"SideRobe.{side}",
            [(sign * .24, .98), (sign * .34, .91), (sign * .30, .29),
             (sign * .20, .40), (sign * .12, .86)],
            .015, .060, mats["cloth_light"],
            {"hips": .70, f"thigh.{side}": .30},
        ))
        pieces.append(add_extruded_polygon(
            f"BackRobe.{side}",
            [(0, .98), (sign * .28, .96), (sign * .31, .30),
             (sign * .12, .17), (0, .40)],
            .160, .040, mats["cloth"], {"hips": .76, f"thigh.{side}": .24},
        ))

    # Arms in a relaxed A-pose, with layered pauldrons and emerald gauntlets.
    for side, sign in (("L", -1), ("R", 1)):
        shoulder = Vector((sign * .285, 0, 1.42))
        elbow = Vector((sign * .385, 0, 1.12))
        wrist = Vector((sign * .455, -.005, .83))
        hand = Vector((sign * .475, -.015, .70))
        pieces.append(add_segment(
            f"UpperArm.{side}", shoulder, elbow, .105, mats["armor"],
            {f"upper_arm.{side}": 1.0}, radius_end=.092,
        ))
        pieces.append(add_segment(
            f"Forearm.{side}", elbow, wrist, .092, mats["cloth_light"],
            {f"forearm.{side}": 1.0}, radius_end=.073,
        ))
        pieces.append(add_segment(
            f"Gauntlet.{side}", Vector((sign * .420, -.008, .98)), wrist,
            .102, mats["graphite"], {f"forearm.{side}": .86, f"hand.{side}": .14},
            radius_end=.080,
        ))
        pieces.append(add_ico(
            f"Hand.{side}", hand, (.072, .062, .105), mats["graphite"],
            {f"hand.{side}": 1.0},
        ))
        pieces.append(add_gem(
            f"GauntletGem.{side}", (sign * .447, -.086, .895), (.035, .018, .070),
            mats["emerald"], {f"forearm.{side}": .92, f"hand.{side}": .08},
        ))
        for layer in range(3):
            z = 1.46 - layer * .055
            x = sign * (.30 + layer * .018)
            pieces.append(add_gem(
                f"Pauldron.{side}.{layer}", (x, -.008, z),
                (.18 - layer * .018, .12, .085),
                mats["graphite"] if layer == 0 else mats["armor"],
                {"chest": .40, f"upper_arm.{side}": .60},
                rotation=(0, sign * .12, sign * .08),
            ))
        pieces.append(add_extruded_polygon(
            f"ShoulderSpike.{side}",
            [(sign * .30, 1.50), (sign * .49, 1.68), (sign * .40, 1.43)],
            .015, .075, mats["trim"], {"chest": .32, f"upper_arm.{side}": .68},
        ))

    # Faceted hood and a true void face, then the emerald V-shaped glyph.
    pieces.append(add_ico(
        "HoodOuter", (0, .010, 1.735), (.30, .255, .355),
        mats["cloth"], {"head": .88, "chest": .12}, subdivisions=2,
    ))
    pieces.append(add_extruded_polygon(
        "VoidFace",
        [(-.155, 1.82), (0, 1.965), (.155, 1.82), (.125, 1.53), (0, 1.46), (-.125, 1.53)],
        -.252, .025, mats["void"], {"head": 1.0},
    ))
    pieces.append(add_extruded_polygon(
        "FaceGlyph.Center",
        [(-.035, 1.84), (0, 1.94), (.035, 1.84), (0, 1.69)],
        -.273, .018, mats["emerald"], {"head": 1.0},
    ))
    pieces.append(add_extruded_polygon(
        "FaceGlyph.Left",
        [(-.132, 1.82), (-.040, 1.72), (0, 1.78), (-.102, 1.91)],
        -.275, .018, mats["emerald"], {"head": 1.0},
    ))
    pieces.append(add_extruded_polygon(
        "FaceGlyph.Right",
        [(0, 1.78), (.040, 1.72), (.132, 1.82), (.102, 1.91)],
        -.275, .018, mats["emerald"], {"head": 1.0},
    ))

    mesh = join_components(pieces, ACTOR["object"])
    mesh["asset_role"] = "body_only"
    mesh["weapons_embedded"] = False
    mesh["procedural_rebuild"] = True
    mesh["concept_reference"] = relative(concept_path)
    return mesh


def skin_procedural_body(mesh, rig):
    """Attach pre-authored component weights without Bone Heat ambiguity."""
    for name in BASE.BODY_BONES:
        if mesh.vertex_groups.get(name) is None:
            mesh.vertex_groups.new(name=name)
    for forbidden in ("root", "weapon_socket.R"):
        group = mesh.vertex_groups.get(forbidden)
        if group is not None:
            mesh.vertex_groups.remove(group)
    mesh.parent = rig
    mesh.matrix_parent_inverse = rig.matrix_world.inverted()
    modifier = mesh.modifiers.new("Runtime Armature", "ARMATURE")
    modifier.object = rig
    modifier.use_deform_preserve_volume = True
    rig.data.bones["weapon_socket.R"].use_deform = True
    mesh.data.validate(clean_customdata=False)
    mesh.data.update()
    return {
        "method": "procedural_component_semantic_weights",
        "boneHeatUsed": False,
        "componentSeamsIntentional": True,
    }


def build_procedural_bow(concept_path):
    """Recreate the graphite/emerald recurve bow as one rigid mesh."""
    bpy.ops.wm.read_factory_settings(use_empty=True)
    BASE.configure_scene()
    mats = archer_materials()
    pieces = []
    upper = [
        (0.0, 0.0, .08), (-.10, 0, .19), (-.185, 0, .36),
        (-.145, 0, .53), (.075, 0, .66),
    ]
    lower = [(x, y, -z) for x, y, z in upper]
    for prefix, points in (("Upper", upper), ("Lower", lower)):
        for index, (start, end) in enumerate(zip(points, points[1:])):
            pieces.append(add_segment(
                f"Bow{prefix}Limb.{index}", start, end,
                .037 - index * .0045, mats["graphite"],
                radius_end=.032 - index * .0045, vertices=6,
            ))
    pieces.append(add_segment(
        "BowGrip", (0, 0, -.105), (0, 0, .105), .048,
        mats["wrap"], radius_end=.048, vertices=8,
    ))
    pieces.append(add_gem(
        "BowGripEmerald", (-.052, -.005, 0), (.055, .030, .070), mats["emerald"],
    ))
    for sign in (-1, 1):
        pieces.append(add_gem(
            f"BowLimbEmerald.{sign}", (-.195, -.003, sign * .355),
            (.050, .028, .075), mats["emerald"],
        ))
        pieces.append(add_gem(
            f"BowTipCrystal.{sign}", (.078, 0, sign * .665),
            (.055, .035, .090), mats["emerald"],
        ))
        pieces.append(add_extruded_polygon(
            f"BowBladePlate.{sign}",
            [(-.10, sign * .19), (-.25, sign * .29), (-.205, sign * .43), (-.125, sign * .35)],
            0, .045, mats["armor"],
        ))
    # Straight string is offset from the grip, matching the turnaround side.
    pieces.append(add_segment(
        "BowString", (.078, 0, -.665), (.078, 0, .665), .006,
        mats["string"], radius_end=.006, vertices=6,
    ))
    mesh = join_components(pieces, "HexArcherBow")
    mesh["asset_role"] = "modular_weapon"
    mesh["owner"] = "hex_archer"
    mesh["pivot_contract"] = "grip_at_local_origin"
    mesh["runtime_grip_offset"] = "identity"
    mesh["rigid"] = True
    mesh["concept_reference"] = relative(concept_path)
    return mesh


def join_imported_meshes(input_path, object_name):
    extension = os.path.splitext(input_path)[1].lower()
    bpy.ops.wm.read_factory_settings(use_empty=True)
    if extension in {".glb", ".gltf"}:
        bpy.ops.import_scene.gltf(filepath=input_path)
    elif extension == ".blend":
        with bpy.data.libraries.load(input_path, link=False) as (source, target):
            target.objects = list(source.objects)
        for obj in target.objects:
            if obj is not None:
                bpy.context.scene.collection.objects.link(obj)
    else:
        raise RuntimeError(f"Unsupported rigid asset input: {input_path}")

    meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    if not meshes:
        raise RuntimeError(f"No mesh found in rigid asset {input_path}")

    for action in list(bpy.data.actions):
        bpy.data.actions.remove(action)
    for mesh in meshes:
        world = mesh.matrix_world.copy()
        mesh.parent = None
        mesh.matrix_world = world
        for modifier in list(mesh.modifiers):
            if modifier.type == "ARMATURE":
                mesh.modifiers.remove(modifier)
        for group in list(mesh.vertex_groups):
            mesh.vertex_groups.remove(group)
        BASE.select_only([mesh], active=mesh)
        bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)

    BASE.select_only(meshes, active=meshes[0])
    if len(meshes) > 1:
        bpy.ops.object.join()
    mesh = bpy.context.view_layer.objects.active
    mesh.name = object_name
    mesh.data.name = object_name + "Mesh"

    for obj in list(bpy.context.scene.objects):
        if obj is not mesh:
            bpy.data.objects.remove(obj, do_unlink=True)
    return mesh


def normalize_bow(mesh, target_span=1.32):
    """Put the longest bow axis on +Z and its physical grip at local origin."""
    bounds = bounds_for_object(mesh)
    dimensions = [
        bounds[1] - bounds[0],
        bounds[3] - bounds[2],
        bounds[5] - bounds[4],
    ]
    longest_axis = max(range(3), key=dimensions.__getitem__)
    if longest_axis != 2:
        source = Vector((1, 0, 0)) if longest_axis == 0 else Vector((0, 1, 0))
        mesh.rotation_mode = "QUATERNION"
        mesh.rotation_quaternion = source.rotation_difference(Vector((0, 0, 1)))
        BASE.select_only([mesh], active=mesh)
        bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)

    bounds = bounds_for_object(mesh)
    span = bounds[5] - bounds[4]
    if span <= 1e-5:
        raise RuntimeError("Hex Archer bow has a degenerate span")
    scale = target_span / span
    mesh.scale = (scale, scale, scale)
    BASE.select_only([mesh], active=mesh)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)

    # The handle is the dense central slice of a bow.  Its median X/Y is more
    # stable than the full silhouette center when one limb has asymmetric VFX.
    bounds = bounds_for_object(mesh)
    center_z = (bounds[4] + bounds[5]) * .5
    span = bounds[5] - bounds[4]
    central = [
        vertex.co.copy() for vertex in mesh.data.vertices
        if abs(vertex.co.z - center_z) <= span * .10
    ]
    if not central:
        central = [vertex.co.copy() for vertex in mesh.data.vertices]
    grip = Vector((
        statistics.median(point.x for point in central),
        statistics.median(point.y for point in central),
        center_z,
    ))
    for vertex in mesh.data.vertices:
        vertex.co -= grip
    mesh.data.update()
    mesh.location = (0, 0, 0)
    mesh.rotation_mode = "XYZ"
    mesh.rotation_euler = (0, 0, 0)
    mesh.scale = (1, 1, 1)

    bounds = bounds_for_object(mesh)
    for minimum, maximum, axis in zip(bounds[::2], bounds[1::2], "XYZ"):
        if minimum > .01 or maximum < -.01:
            raise RuntimeError(
                f"Bow grip origin falls outside {axis} bounds [{minimum:.5f}, {maximum:.5f}]"
            )
    if abs((bounds[5] - bounds[4]) - target_span) > .003:
        raise RuntimeError("Bow normalization did not preserve the target span")

    mesh["asset_role"] = "modular_weapon"
    mesh["owner"] = "hex_archer"
    mesh["pivot_contract"] = "grip_at_local_origin"
    mesh["runtime_grip_offset"] = "identity"
    mesh["rigid"] = True
    mesh["source_axis"] = "+Z_up_-Y_forward"
    return bounds


def export_rigid_bow(mesh, output_path):
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    BASE.pack_images()
    BASE.select_only([mesh], active=mesh)
    bpy.ops.export_scene.gltf(
        filepath=output_path,
        export_format="GLB",
        use_selection=True,
        export_yup=True,
        export_animations=False,
        export_skins=False,
        export_morph=False,
        export_materials="EXPORT",
        export_texcoords=True,
        export_normals=True,
        export_tangents=False,
        export_cameras=False,
        export_lights=False,
        export_extras=True,
        export_apply=True,
    )


def validate_rigid_bow(path):
    document = BASE.read_glb_json(path)
    if document.get("skins"):
        raise RuntimeError("Rigid bow GLB must not contain a skin")
    if document.get("animations"):
        raise RuntimeError("Rigid bow GLB must not contain animation")
    nodes = document.get("nodes", [])
    mesh_nodes = [node for node in nodes if "mesh" in node]
    if len(mesh_nodes) != 1:
        raise RuntimeError(f"Rigid bow must export one mesh node; found {len(mesh_nodes)}")
    primitives = []
    for mesh in document.get("meshes", []):
        primitives.extend(mesh.get("primitives", []))
    if not primitives:
        raise RuntimeError("Rigid bow GLB contains no mesh primitive")
    forbidden = []
    for primitive in primitives:
        attributes = primitive.get("attributes", {})
        forbidden.extend(
            name for name in attributes if name.startswith("JOINTS") or name.startswith("WEIGHTS")
        )
    if forbidden:
        raise RuntimeError(f"Rigid bow unexpectedly contains skin attributes: {forbidden}")
    return {
        "meshNodeCount": len(mesh_nodes),
        "primitiveCount": len(primitives),
        "skinCount": 0,
        "animationCount": 0,
        "rigid": True,
    }


def build_bow(concept_path, output_path):
    print(f"\n=== HEX ARCHER MODULAR BOW ===\nconcept={concept_path}", flush=True)
    mesh = build_procedural_bow(concept_path)
    bounds = normalize_bow(mesh)
    if not mesh.data.materials:
        raise RuntimeError("Hex Archer bow source has no material")
    export_rigid_bow(mesh, output_path)
    glb_audit = validate_rigid_bow(output_path)
    triangles = sum(len(polygon.vertices) - 2 for polygon in mesh.data.polygons)
    result = {
        "status": "complete",
        "concept": relative(concept_path),
        "proceduralRebuild": True,
        "output": relative(output_path),
        "object": mesh.name,
        "vertices": len(mesh.data.vertices),
        "triangles": triangles,
        "boundsMeters": [
            round(bounds[1] - bounds[0], 5),
            round(bounds[3] - bounds[2], 5),
            round(bounds[5] - bounds[4], 5),
        ],
        "pivot": "grip_at_local_origin",
        "runtimeGripOffset": "identity",
        "glbAudit": glb_audit,
    }
    print("HEX_ARCHER_BOW_RESULT=" + json.dumps(result), flush=True)
    return result


def remove_keys(action, first, last):
    for curve in action.fcurves:
        for point in list(curve.keyframe_points):
            if first <= point.co.x <= last:
                curve.keyframe_points.remove(point)


def draw_pose(height, strength=1.0, crouch=0.0, twist=0.0):
    """Right hand holds the bow; left hand pulls the spectral string."""
    return {
        "hips.loc": (0, 0, -crouch * height * .022),
        "hips.rot": (0, 0, twist * .18),
        "chest.rot": (-.04 * strength, 0, -.12 * strength + twist * .20),
        "head.rot": (.02, 0, .09 * strength),
        # Right hand presents the bow at shoulder height.  Local Z is the
        # visible A-pose-to-horizontal swing for this generated skeleton.
        "upper_arm.R.rot": (-.24 * strength, -.04, .92 * strength),
        "forearm.R.rot": (-.10 * strength, 0, .10 * strength),
        "hand.R.rot": (0, -.10 * strength, -.08 * strength),
        # Left elbow opens, while the forearm folds back so the hand reaches
        # the hood/cheek and clearly reads as the string hand.
        "upper_arm.L.rot": (-.24 * strength, .04, -.92 * strength),
        "forearm.L.rot": (-.30 * strength, 0, -1.12 * strength),
        "hand.L.rot": (0, .12 * strength, .24 * strength),
        "thigh.L.rot": (-.10 * crouch, 0, 0),
        "thigh.R.rot": (.08 * crouch, 0, 0),
        "shin.L.rot": (-.18 * crouch, 0, 0),
        "shin.R.rot": (-.12 * crouch, 0, 0),
    }


def release_pose(height, power=1.0, twist=0.0):
    return {
        "hips.loc": (0, -.014 * height * power, .005 * height * power),
        "hips.rot": (0, 0, twist * .20),
        "chest.rot": (.10 * power, 0, .18 * power + twist * .22),
        "head.rot": (-.04 * power, 0, -.08 * power),
        "upper_arm.R.rot": (-.18 * power, 0, .86 * power),
        "forearm.R.rot": (-.08, 0, .10),
        "hand.R.rot": (0, -.05, -.06),
        "upper_arm.L.rot": (-.12 * power, 0, -.76 * power),
        "forearm.L.rot": (-.08 * power, 0, -.32 * power),
        "hand.L.rot": (0, -.15 * power, -.20 * power),
    }


def key_archery_clip(rig, action, first, last, poses):
    remove_keys(action, first, last)
    for frame, pose in poses:
        BASE.key_pose(rig, frame, pose)


def archer_socket_animation(rig):
    # The processed bow is +Z-up with its grip at the origin.  The identity
    # socket keeps it upright; small rolls preserve a readable silhouette.
    pose_frames = {
        0: (0, 0, -.12), 24: (0, 0, -.11), 47: (0, 0, -.12),
        71: (0, 0, -.13), 94: (0, 0, -.12),
        96: (0, 0, -.18), 104: (0, 0, -.20), 112: (0, 0, -.16),
        120: (0, 0, -.20), 127: (0, 0, -.18), 135: (0, 0, -.20),
        143: (0, 0, -.16), 151: (0, 0, -.20), 158: (0, 0, -.18),
        160: (0, 0, -.12), 172: (0, -.05, -.06), 184: (0, -.08, 0),
        196: (0, -.10, .02), 202: (0, .04, -.08), 212: (0, 0, -.12), 222: (0, 0, -.12),
        224: (0, 0, -.12), 242: (0, -.08, -.02), 260: (0, -.14, .04),
        274: (0, .06, -.10), 286: (0, -.10, .03), 302: (0, 0, -.12),
        304: (0, 0, -.12), 324: (0, -.18, -.18), 344: (0, .12, .08),
        364: (0, 0, -.16), 382: (0, 0, -.12),
        384: (0, 0, -.12), 408: (0, -.12, -.02), 432: (0, -.20, .08),
        448: (0, .18, -.12), 466: (0, 0, -.16), 478: (0, 0, -.12),
        480: (0, 0, -.16), 492: (0, 0, -.20), 505: (0, 0, -.28),
        518: (0, 0, -.18), 526: (0, 0, -.12),
        528: (0, 0, -.12), 540: (0, 0, .24), 550: (0, 0, -.02), 558: (0, 0, -.12),
        560: (0, 0, -.12), 584: (0, 0, -.08), 612: (0, 0, 0),
        638: (0, 0, 0), 654: (0, 0, 0),
    }
    for frame, rotation in pose_frames.items():
        # Blender edit bones point along local Y.  Map the bow's authored +Z
        # span back to model/world up, exactly like the modular Caster staff.
        BASE.key_weapon_socket(
            rig,
            frame,
            (-math.pi * .5 + rotation[0], rotation[1], rotation[2]),
        )


def add_archery_ik_controllers(rig, height):
    """Place the bow hand at shoulder height and the string hand by the cheek.

    The controllers remain in the editable .blend.  Blender's glTF exporter
    force-samples the evaluated pose, so runtime GLB clips receive baked joint
    transforms without depending on Blender constraints or controller nodes.
    """
    controller_specs = {
        "R": {
            "bone": "forearm.R",
            "location": Vector((.205 * height, -.11 * height, .675 * height)),
            "pole": Vector((.29 * height, .16 * height, .58 * height)),
        },
        "L": {
            "bone": "forearm.L",
            "location": Vector((-.012 * height, -.17 * height, .805 * height)),
            "pole": Vector((-.29 * height, .16 * height, .60 * height)),
        },
    }
    controllers = {}
    constraints = {}
    for side, spec in controller_specs.items():
        target = bpy.data.objects.new(f"CTRL_ArcherHandIK.{side}", None)
        target.empty_display_type = "SPHERE"
        target.empty_display_size = .045 * height
        target.location = spec["location"]
        target["animation_controller"] = True
        bpy.context.scene.collection.objects.link(target)
        pole = bpy.data.objects.new(f"CTRL_ArcherElbowPole.{side}", None)
        pole.empty_display_type = "CUBE"
        pole.empty_display_size = .035 * height
        pole.location = spec["pole"]
        pole["animation_controller"] = True
        bpy.context.scene.collection.objects.link(pole)
        constraint = rig.pose.bones[spec["bone"]].constraints.new("IK")
        constraint.name = f"ArcherHandIK.{side}"
        constraint.target = target
        constraint.pole_target = pole
        constraint.chain_count = 2
        constraint.use_tail = True
        constraint.influence = 0.0
        controllers[side] = target
        constraints[side] = constraint

    neutral_right = Vector((.205 * height, -.08 * height, .675 * height))
    neutral_left = Vector((-.205 * height, -.08 * height, .675 * height))
    bow_draw = Vector((.235 * height, -.12 * height, .695 * height))
    string_draw = Vector((-.020 * height, -.19 * height, .805 * height))
    bow_release = Vector((.245 * height, -.13 * height, .700 * height))
    string_release = Vector((-.205 * height, -.16 * height, .720 * height))

    keyed = [
        (0, 0.0, neutral_right, neutral_left),
        (94, 0.0, neutral_right, neutral_left),
        (96, 0.0, neutral_right, neutral_left),
        (158, 0.0, neutral_right, neutral_left),
        (160, 0.0, neutral_right, neutral_left),
        (172, .65, bow_draw, string_draw),
        (184, 1.0, bow_draw, string_draw),
        (196, 1.0, bow_draw, string_draw),
        (202, 1.0, bow_release, string_release),
        (212, .45, bow_release, string_release),
        (222, 0.0, neutral_right, neutral_left),
        (224, 0.0, neutral_right, neutral_left),
        (242, .70, bow_draw, string_draw),
        (260, 1.0, bow_draw, string_draw),
        (266, 1.0, bow_release, string_release),
        (274, 1.0, bow_draw, string_draw),
        (280, 1.0, bow_release, string_release),
        (286, 1.0, bow_draw, string_draw),
        (292, 1.0, bow_release, string_release),
        (302, 0.0, neutral_right, neutral_left),
        (304, 0.0, neutral_right, neutral_left),
        (324, .78, bow_draw, string_draw),
        (336, 1.0, bow_draw, string_draw),
        (344, 1.0, bow_release, string_release),
        (364, .35, bow_release, string_release),
        (382, 0.0, neutral_right, neutral_left),
        (384, 0.0, neutral_right, neutral_left),
        (408, .72, bow_draw, string_draw),
        (432, 1.0, bow_draw + Vector((0, 0, .035 * height)),
         string_draw + Vector((0, 0, .025 * height))),
        (442, 1.0, bow_draw + Vector((0, 0, .035 * height)),
         string_draw + Vector((0, 0, .025 * height))),
        (448, 1.0, bow_release, string_release),
        (466, .40, bow_release, string_release),
        (478, 0.0, neutral_right, neutral_left),
        (480, 0.0, neutral_right, neutral_left),
        (654, 0.0, neutral_right, neutral_left),
    ]
    for frame, influence, right, left in keyed:
        controllers["R"].location = right
        controllers["L"].location = left
        controllers["R"].keyframe_insert("location", frame=frame)
        controllers["L"].keyframe_insert("location", frame=frame)
        for constraint in constraints.values():
            constraint.influence = influence
            constraint.keyframe_insert("influence", frame=frame)

    for controller in controllers.values():
        if controller.animation_data and controller.animation_data.action:
            for curve in controller.animation_data.action.fcurves:
                for key in curve.keyframe_points:
                    key.interpolation = "BEZIER"
                    key.handle_left_type = "AUTO_CLAMPED"
                    key.handle_right_type = "AUTO_CLAMPED"
    return {
        "controllers": [controller.name for controller in controllers.values()],
        "constraints": [constraint.name for constraint in constraints.values()],
        "export": "force_sampled_visual_pose",
    }


def create_archer_animation(mesh, rig):
    for action in list(bpy.data.actions):
        bpy.data.actions.remove(action)
    height = BASE.mesh_bounds(mesh)[5] - BASE.mesh_bounds(mesh)[4]
    action = bpy.data.actions.new("hex_archer_Master_60FPS")
    rig.animation_data_create()
    rig.animation_data.action = action

    # Proven locomotion, hit, death and grounding base; all combat clips below
    # are replaced, rather than layered over the Caster gestures.
    BASE.humanoid_animation(ACTOR, rig, height)
    key_archery_clip(rig, action, 160, 222, [
        (160, draw_pose(height, .18)),
        (172, draw_pose(height, .54, .25)),
        (184, draw_pose(height, 1.00, .45)),
        (196, draw_pose(height, 1.08, .55)),
        (202, release_pose(height, 1.00)),
        (212, release_pose(height, .38)),
        (222, draw_pose(height, .18)),
    ])
    key_archery_clip(rig, action, 224, 302, [
        (224, draw_pose(height, .18)),
        (242, draw_pose(height, .66, .35, -.20)),
        (260, draw_pose(height, 1.08, .55, -.30)),
        (266, release_pose(height, .78, -.25)),
        (274, draw_pose(height, 1.02, .48, 0)),
        (280, release_pose(height, .84, 0)),
        (286, draw_pose(height, .96, .42, .30)),
        (292, release_pose(height, .76, .25)),
        (302, draw_pose(height, .18)),
    ])
    key_archery_clip(rig, action, 304, 382, [
        (304, draw_pose(height, .18)),
        (324, draw_pose(height, .78, .70, -.65)),
        (336, draw_pose(height, 1.12, .60, .55)),
        (344, release_pose(height, 1.20, .75)),
        (364, release_pose(height, .34, .20)),
        (382, draw_pose(height, .18)),
    ])
    key_archery_clip(rig, action, 384, 478, [
        (384, draw_pose(height, .18)),
        (408, draw_pose(height, .72, .60, -.18)),
        (432, draw_pose(height, 1.28, .82, 0)),
        (442, draw_pose(height, 1.36, .90, 0)),
        (448, release_pose(height, 1.42, .05)),
        (466, release_pose(height, .42)),
        (478, draw_pose(height, .18)),
    ])
    archer_socket_animation(rig)
    rig["archery_ik"] = json.dumps(add_archery_ik_controllers(rig, height))

    for curve in action.fcurves:
        for key in curve.keyframe_points:
            key.interpolation = "BEZIER"
            key.handle_left_type = "AUTO_CLAMPED"
            key.handle_right_type = "AUTO_CLAMPED"
    BASE.configure_scene()
    return action


def validate_body_animation(path):
    audit = BASE.validate_exported_glb(path)
    document = BASE.read_glb_json(path)
    animations = document.get("animations", [])
    if len(animations) != 1:
        raise RuntimeError(f"Expected one Archer animation; found {len(animations)}")
    accessors = document.get("accessors", [])
    time_accessors = [
        accessors[sampler["input"]]
        for sampler in animations[0].get("samplers", [])
    ]
    if not time_accessors:
        raise RuntimeError("Archer animation has no time samplers")
    sample_counts = {accessor.get("count", 0) for accessor in time_accessors}
    maximum_time = max(accessor.get("max", [0.0])[0] for accessor in time_accessors)
    # Blender reduces channels that remain constant for the entire master
    # action to two endpoints.  At least one changing channel must retain the
    # complete 655-frame timeline; every other channel is valid with 2..655.
    if 655 not in sample_counts or min(sample_counts) < 2 or max(sample_counts) > 655:
        raise RuntimeError(
            "Archer animation needs a full 655-frame master timeline and only "
            f"valid reduced channels; got {sample_counts}"
        )
    if abs(maximum_time - 10.9) > .002:
        raise RuntimeError(f"Archer animation duration must be 10.9s; got {maximum_time:.6f}s")
    audit.update({
        "animationCount": 1,
        "sampleCounts": sorted(sample_counts),
        "frames": 655,
        "durationSeconds": round(maximum_time, 5),
    })
    return audit


def process_body(body_concept, bow_output, paths, no_render):
    print(f"\n=== HEX ARCHER MODULAR BODY ===\nconcept={body_concept}", flush=True)
    BASE.QA_ROOT = paths["qa"]
    mesh = build_procedural_body(body_concept)
    bounds = BASE.normalize_body(ACTOR, mesh)
    BASE.validate_builder_surface(ACTOR, mesh)
    geometry = audit_archer_body_only_geometry(mesh, bounds)
    mesh.name = ACTOR["object"]
    mesh.data.name = ACTOR["object"] + "Mesh"
    mesh["asset_role"] = "body_only"
    mesh["weapons_embedded"] = False
    mesh["source_builder"] = "procedural_low_poly_from_turnaround"
    mesh["concept_reference"] = relative(body_concept)

    rig, specs = BASE.create_rig(ACTOR, mesh)
    rig["joint_contract"] = "modular_archer_16_v1"
    skinning = skin_procedural_body(mesh, rig)
    BASE.validate_rig(rig)
    weights = BASE.audit_weights(mesh)
    if weights["unweightedVertices"]:
        raise RuntimeError(f"Archer body has {weights['unweightedVertices']} unweighted vertices")
    if weights["forbiddenSocketWeightedVertices"]:
        raise RuntimeError("Archer body vertices must never be weighted to weapon_socket.R")
    if weights["maximumInfluences"] > 4 or weights["minimumWeightSum"] < .999:
        raise RuntimeError(f"Archer body violates four-normalized-influence contract: {weights}")

    modifier = next(item for item in mesh.modifiers if item.type == "ARMATURE")
    modifier.show_viewport = False
    action = create_archer_animation(mesh, rig)
    modifier.show_viewport = True
    grounding = BASE.bake_grounding(ACTOR, mesh, rig, action)
    deformation = BASE.audit_deformation(ACTOR, mesh)

    old_resolver = BASE.resolve_weapon_input
    BASE.resolve_weapon_input = lambda _actor: bow_output
    try:
        qa_bow, bow_qa = BASE.load_qa_weapon(ACTOR)
    finally:
        BASE.resolve_weapon_input = old_resolver
    qa_bow.hide_render = True
    qa = []
    if not no_render:
        for frame, label, three_quarter, rear_view in QA_POSES:
            qa.append(BASE.render_qa(
                ACTOR,
                mesh,
                rig,
                frame,
                label,
                three_quarter=three_quarter,
                rear_view=rear_view,
                weapon=qa_bow,
            ))
    BASE.remove_qa_weapon(qa_bow)

    os.makedirs(os.path.dirname(paths["blend"]), exist_ok=True)
    os.makedirs(os.path.dirname(paths["body"]), exist_ok=True)
    old_output_paths = BASE.output_paths
    BASE.output_paths = lambda _actor: (paths["blend"], paths["body"])
    try:
        blend_path, glb_path = BASE.export_actor(ACTOR, mesh, rig)
    finally:
        BASE.output_paths = old_output_paths
    glb_audit = validate_body_animation(glb_path)
    triangles = sum(len(polygon.vertices) - 2 for polygon in mesh.data.polygons)
    result = {
        "status": "complete",
        "concept": relative(body_concept),
        "proceduralRebuild": True,
        "output": relative(glb_path),
        "blend": relative(blend_path),
        "object": mesh.name,
        "bodyOnly": True,
        "embeddedWeaponGeometry": False,
        "vertices": len(mesh.data.vertices),
        "triangles": triangles,
        "boundsMeters": [
            round(bounds[1] - bounds[0], 5),
            round(bounds[3] - bounds[2], 5),
            round(bounds[5] - bounds[4], 5),
        ],
        "bodyGeometry": geometry,
        "joints": len(rig.data.bones),
        "jointNames": [bone.name for bone in rig.data.bones],
        "socket": {"name": "weapon_socket.R", "parent": "hand.R"},
        "weaponSocketWeights": 0,
        "weaponPivotContract": "grip_at_local_origin",
        "frames": [int(action.frame_range[0]), int(action.frame_range[1])],
        "durationSeconds": 10.9,
        "clips": [dict(id=name, start=first, end=last) for name, first, last in BASE.CLIPS],
        "combatEvents": COMBAT_EVENTS,
        "weights": weights,
        "skinning": skinning,
        "grounding": grounding,
        "deformation": deformation,
        "bowQa": bow_qa,
        "qa": qa,
        "glbAudit": glb_audit,
    }
    print("HEX_ARCHER_BODY_RESULT=" + json.dumps(result), flush=True)
    return result


def main():
    args = parse_args()
    paths = output_paths(args.output_root)
    missing = [
        path for path in (args.body_concept, args.bow_concept)
        if not os.path.isfile(path)
    ]
    if missing:
        report = {
            "schemaVersion": 1,
            "pipeline": "hex_archer_modular_body_bow_v1",
            "status": "pending_input",
            "expectedConcepts": [
                relative(path) for path in (args.body_concept, args.bow_concept)
            ],
            "missingInputs": [relative(path) for path in missing],
            "outputs": {key: relative(value) for key, value in paths.items() if key != "qa"},
        }
        write_report(paths["report"], report)
        print("HEX_ARCHER_MODULAR_REPORT=" + json.dumps(report), flush=True)
        if args.strict_missing:
            raise SystemExit(2)
        return

    bow_result = build_bow(args.bow_concept, paths["bow"])
    body_result = process_body(
        args.body_concept, paths["bow"], paths, args.no_render
    )
    report = {
        "schemaVersion": 1,
        "pipeline": "hex_archer_modular_body_bow_v1",
        "status": "complete",
        "sampleRate": 60,
        "totalFrames": 655,
        "durationSeconds": 10.9,
        "jointContract": {
            "id": "modular_archer_16_v1",
            "count": 16,
            "names": BASE.EXPECTED_JOINTS,
            "weaponSocket": "weapon_socket.R",
            "weaponSocketParent": "hand.R",
            "bodySocketWeights": 0,
        },
        "weaponContract": {
            "separateGlb": True,
            "rigid": True,
            "skinCount": 0,
            "animationCount": 0,
            "pivot": "grip_at_local_origin",
            "runtimeGripOffset": "identity",
        },
        "clips": [dict(id=name, start=first, end=last) for name, first, last in BASE.CLIPS],
        "combatEvents": COMBAT_EVENTS,
        "body": body_result,
        "bow": bow_result,
    }
    write_report(paths["report"], report)
    print("HEX_ARCHER_MODULAR_REPORT=" + json.dumps(report), flush=True)


if __name__ == "__main__":
    try:
        main()
    except SystemExit:
        raise
    except Exception:
        traceback.print_exc()
        raise SystemExit(1)
