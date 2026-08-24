"""Rig, skin, animate, QA, and export the production-v3 modular hero bodies.

This pipeline intentionally consumes only the body assets produced by the
production-v3 modular asset builder.  Weapons remain independent assets.  A
non-body-weighted ``weapon_socket.R`` joint is exported as part of the skin so
the runtime can attach any compatible weapon without baking weapon geometry
into the character mesh.

Run from the repository root with Blender 4.5 LTS::

    blender -b --python tools/rig_animate_modular_heroes.py -- --actor all

Missing builder inputs are reported as pending and skipped.  Pass
``--strict-missing`` when a build should fail until every requested input is
present.  ``--no-render`` is available for a fast structural smoke test.
"""

import argparse
import bpy
import bmesh
import json
import math
import os
import statistics
import struct
import sys
from mathutils import Matrix, Quaternion, Vector


ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
ASSETS = os.path.join(ROOT, "assets", "survival3d")
PRODUCTION = os.path.join(ASSETS, "production_v3")
OUTPUT_ROOT = PRODUCTION
QA_ROOT = os.path.join(OUTPUT_ROOT, "qa", "rigged")
REPORT_PATH = os.path.join(OUTPUT_ROOT, "modular_hero_rig_report.json")

CLIPS = [
    ("idle", 0, 94),
    ("run", 96, 158),
    ("basic", 160, 222),
    ("skillOne", 224, 302),
    ("skillTwo", 304, 382),
    ("ultimatePhase", 384, 478),
    ("dashSpecial", 480, 526),
    ("hurt", 528, 558),
    ("death", 560, 654),
]

QA_POSES = [
    (0, "rest"),
    (47, "idle"),
    (104, "run"),
    (190, "basic"),
    (274, "skill_one"),
    (344, "skill_two"),
    (412, "ultimate_windup"),
    (448, "ultimate"),
    (505, "dash"),
    (540, "hurt"),
    (638, "death"),
]

# Contact sheets for Step 6 deliberately sample the three animation beats that
# have to remain readable after the runtime samples the 60 FPS master action.
# These are rendered with the modular weapon equipped, unlike the legacy QA
# poses above, so a sword/staff that appears to move independently of the body
# is immediately visible.
COMBAT_QA_POSES = [
    (168, "basic_windup"),
    (188, "basic_contact"),
    (206, "basic_recovery"),
    (242, "skill_one_windup"),
    (274, "skill_one_contact"),
    (290, "skill_one_recovery"),
    (324, "skill_two_windup"),
    (344, "skill_two_contact"),
    (364, "skill_two_recovery"),
    (408, "ultimate_windup_step6"),
    (448, "ultimate_contact"),
    (466, "ultimate_recovery"),
    (492, "dash_windup"),
    (505, "dash_contact"),
    (518, "dash_recovery"),
]

COMBAT_CONTACTS = [
    ("basic", 188),
    ("skillOne", 274),
    ("skillTwo", 344),
    ("ultimatePhase", 448),
    ("dashSpecial", 505),
]

ACTORS = {
    "knight": {
        "id": "knight",
        "style": "knight",
        "object": "KnightBody",
        "body_height": 2.0,
        "input_stem": "knight_body_production_v3",
        "weapon_input_stem": "knight_greatsword_production_v3",
        "output_blend": "knight_body_rigged_production_v3.blend",
        "output_glb": "knight_body_animated_production_v3.glb",
        "weapon_grip_ratio": .55,
        "weapon_grip_depth": 0.0,
    },
    "magic_caster": {
        "id": "magic_caster",
        "style": "caster",
        "object": "MagicCasterBody",
        "body_height": 2.0,
        "input_stem": "magic_caster_body_production_v3",
        "weapon_input_stem": "magic_caster_staff_production_v3",
        "output_blend": "magic_caster_body_rigged_production_v3.blend",
        "output_glb": "magic_caster_body_animated_production_v3.glb",
        "weapon_grip_ratio": .92,
        "weapon_grip_depth": 0.0,
    },
}

BODY_BONES = [
    "hips",
    "spine",
    "chest",
    "head",
    "upper_arm.L",
    "forearm.L",
    "hand.L",
    "upper_arm.R",
    "forearm.R",
    "hand.R",
    "thigh.L",
    "shin.L",
    "thigh.R",
    "shin.R",
]
EXPECTED_JOINTS = ["root", *BODY_BONES, "weapon_socket.R"]
WEAPON_WORDS = ("weapon", "sword", "greatsword", "staff", "blade")


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--actor",
        choices=("all", *ACTORS.keys()),
        default="all",
        help="Hero body to process; defaults to both heroes.",
    )
    parser.add_argument(
        "--input",
        help="Optional body .blend/.glb override; valid only for one actor.",
    )
    parser.add_argument(
        "--no-render",
        action="store_true",
        help="Skip PNG pose renders while retaining structural and GLB audits.",
    )
    parser.add_argument(
        "--strict-missing",
        action="store_true",
        help="Fail if any requested production-v3 builder input is absent.",
    )
    parser.add_argument(
        "--output-root",
        help=(
            "Optional staging directory for rigged .blend/.glb, QA renders, and "
            "the report. Builder inputs still come from production_v3 unless "
            "--input is supplied."
        ),
    )
    values = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    args = parser.parse_args(values)
    if args.input and args.actor == "all":
        parser.error("--input requires --actor knight or --actor magic_caster")
    return args


def relative(path):
    return os.path.relpath(path, ROOT).replace("\\", "/")


def actor_directory(actor):
    return os.path.join(PRODUCTION, "heroes", actor["id"])


def input_candidates(actor):
    directory = actor_directory(actor)
    stem = actor["input_stem"]
    return [os.path.join(directory, stem + extension) for extension in (".blend", ".glb")]


def weapon_input_candidates(actor):
    directory = actor_directory(actor)
    stem = actor["weapon_input_stem"]
    return [os.path.join(directory, stem + extension) for extension in (".blend", ".glb")]


def resolve_weapon_input(actor):
    return next(
        (path for path in weapon_input_candidates(actor) if os.path.isfile(path)),
        None,
    )


def output_paths(actor):
    directory = os.path.join(OUTPUT_ROOT, "heroes", actor["id"])
    return (
        os.path.join(directory, actor["output_blend"]),
        os.path.join(directory, actor["output_glb"]),
    )


def resolve_input(actor, override=None):
    if override:
        override = os.path.abspath(os.path.join(ROOT, override))
        return override if os.path.isfile(override) else None
    return next((path for path in input_candidates(actor) if os.path.isfile(path)), None)


def select_only(objects, active=None):
    bpy.ops.object.mode_set(mode="OBJECT") if bpy.context.object and bpy.context.object.mode != "OBJECT" else None
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    if objects:
        bpy.context.view_layer.objects.active = active or objects[0]


def mesh_bounds(mesh):
    points = [mesh.matrix_world @ vertex.co for vertex in mesh.data.vertices]
    if not points:
        raise RuntimeError(f"Body mesh {mesh.name!r} has no vertices")
    return (
        min(point.x for point in points),
        max(point.x for point in points),
        min(point.y for point in points),
        max(point.y for point in points),
        min(point.z for point in points),
        max(point.z for point in points),
    )


def body_center(mesh):
    xmin, xmax, ymin, ymax, zmin, zmax = mesh_bounds(mesh)
    height = zmax - zmin
    central = [
        mesh.matrix_world @ vertex.co
        for vertex in mesh.data.vertices
        if zmin + height * .18 < (mesh.matrix_world @ vertex.co).z < zmin + height * .88
    ]
    if not central:
        central = [mesh.matrix_world @ vertex.co for vertex in mesh.data.vertices]
    return statistics.median(point.x for point in central), statistics.median(point.y for point in central)


def configure_scene():
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    scene.frame_start = 0
    scene.frame_end = 654
    scene.render.fps = 60
    scene.render.engine = "BLENDER_EEVEE_NEXT"
    scene.view_settings.view_transform = "AgX"
    scene.view_settings.look = "AgX - Medium High Contrast"
    scene.timeline_markers.clear()
    for name, first, last in CLIPS:
        scene.timeline_markers.new(name.upper() + " START", frame=first)
        scene.timeline_markers.new(name.upper() + " END", frame=last)


def load_body(actor, input_path):
    extension = os.path.splitext(input_path)[1].lower()
    if extension == ".blend":
        bpy.ops.wm.open_mainfile(filepath=input_path)
    elif extension in {".glb", ".gltf"}:
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.gltf(filepath=input_path)
    else:
        raise RuntimeError(f"Unsupported builder input: {input_path}")

    configure_scene()
    meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    if not meshes:
        raise RuntimeError(f"No body mesh in {input_path}")

    weapon_named = [
        obj.name for obj in meshes
        if any(word in obj.name.lower() for word in WEAPON_WORDS)
    ]
    if weapon_named:
        raise RuntimeError(
            f"Body input illegally contains weapon-like mesh objects: {weapon_named}"
        )

    mesh = next((obj for obj in meshes if obj.name == actor["object"]), None)
    if mesh is None and len(meshes) == 1:
        mesh = meshes[0]
        print(
            f"renaming sole builder mesh {mesh.name!r} -> {actor['object']!r}",
            flush=True,
        )
        mesh.name = actor["object"]
    if mesh is None:
        raise RuntimeError(
            f"Expected builder object {actor['object']!r}; found {[obj.name for obj in meshes]}"
        )
    if len(meshes) != 1:
        raise RuntimeError(
            f"Production-v3 body contract requires one mesh; found {[obj.name for obj in meshes]}"
        )

    # A rigged source file produced by an earlier run is never a valid builder
    # input.  Clear any stale rig/action state defensively while preserving the
    # production material and UV data.
    world_matrix = mesh.matrix_world.copy()
    mesh.parent = None
    mesh.matrix_world = world_matrix
    for modifier in list(mesh.modifiers):
        if modifier.type == "ARMATURE":
            mesh.modifiers.remove(modifier)
    for group in list(mesh.vertex_groups):
        mesh.vertex_groups.remove(group)
    for obj in list(bpy.data.objects):
        if obj is not mesh:
            bpy.data.objects.remove(obj, do_unlink=True)
    for action in list(bpy.data.actions):
        bpy.data.actions.remove(action)

    select_only([mesh])
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    return mesh


def normalize_body(actor, mesh):
    xmin, xmax, ymin, ymax, zmin, zmax = mesh_bounds(mesh)
    height = zmax - zmin
    target_height = actor["body_height"]
    if height <= 1e-5:
        raise RuntimeError(f"Degenerate body height for {actor['id']}")
    scale = target_height / height
    mesh.scale = (scale, scale, scale)
    select_only([mesh])
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)

    _, _, _, _, zmin, _ = mesh_bounds(mesh)
    center_x, center_y = body_center(mesh)
    mesh.location += Vector((-center_x, -center_y, -zmin))
    select_only([mesh])
    bpy.ops.object.transform_apply(location=True, rotation=False, scale=False)

    bounds = mesh_bounds(mesh)
    normalized_height = bounds[5] - bounds[4]
    if abs(normalized_height - target_height) > .002:
        raise RuntimeError(
            f"{actor['id']} normalization failed: {normalized_height:.6f}m"
        )
    if abs(bounds[4]) > 1e-5:
        raise RuntimeError(f"{actor['id']} is not grounded: minZ={bounds[4]:.7f}")

    mesh.location = (0, 0, 0)
    mesh.rotation_euler = (0, 0, 0)
    mesh.scale = (1, 1, 1)
    mesh.data.validate(clean_customdata=False)
    mesh.data.update()
    return bounds


def validate_builder_surface(actor, mesh):
    uv_names = [layer.name for layer in mesh.data.uv_layers]
    if uv_names != ["UVMap"]:
        raise RuntimeError(
            f"{actor['id']} must have exactly one UV layer named UVMap; got {uv_names}"
        )
    if not mesh.data.materials:
        raise RuntimeError(f"{actor['id']} has no production-v3 material")
    invalid_materials = [
        material.name for material in mesh.data.materials
        if material and any(word in material.name.lower() for word in WEAPON_WORDS)
    ]
    if invalid_materials:
        raise RuntimeError(
            f"Body material names suggest embedded weapon data: {invalid_materials}"
        )


def connected_component_sizes(mesh):
    neighbours = [[] for _ in mesh.data.vertices]
    for edge in mesh.data.edges:
        left, right = edge.vertices
        neighbours[left].append(right)
        neighbours[right].append(left)
    unseen = set(range(len(mesh.data.vertices)))
    sizes = []
    while unseen:
        seed = unseen.pop()
        stack = [seed]
        size = 0
        while stack:
            vertex = stack.pop()
            size += 1
            for neighbour in neighbours[vertex]:
                if neighbour in unseen:
                    unseen.remove(neighbour)
                    stack.append(neighbour)
        sizes.append(size)
    return sorted(sizes, reverse=True)


def audit_body_only_geometry(actor, mesh, bounds):
    width = bounds[1] - bounds[0]
    depth = bounds[3] - bounds[2]
    height = bounds[5] - bounds[4]
    components = connected_component_sizes(mesh)
    major_threshold = max(32, int(len(mesh.data.vertices) * .02))
    major_components = [size for size in components if size >= major_threshold]

    # Both concepts fit comfortably inside this silhouette.  A forgotten or
    # fused sword/staff makes one of these ratios jump well past the limit even
    # if its object/material was renamed to look like body geometry.
    if width / height > .72 or depth / height > .58:
        raise RuntimeError(
            f"{actor['id']} body envelope suggests embedded weapon geometry: "
            f"width/height={width / height:.4f}, depth/height={depth / height:.4f}"
        )
    if len(major_components) > 1:
        raise RuntimeError(
            f"{actor['id']} contains multiple major mesh components "
            f"{major_components}; body and weapon must be separate assets"
        )
    return {
        "componentCount": len(components),
        "majorComponentCount": len(major_components),
        "largestComponentVertices": components[0] if components else 0,
        "widthToHeight": round(width / height, 5),
        "depthToHeight": round(depth / height, 5),
    }


def humanoid_specs(mesh, weapon_grip_ratio=.55, weapon_grip_depth=0.0):
    xmin, xmax, ymin, ymax, zmin, zmax = mesh_bounds(mesh)
    height = zmax - zmin
    center_x, center_y = body_center(mesh)
    shoulder = .105 * height
    elbow = .170 * height
    wrist = .222 * height
    hand_tip = .242 * height
    specs = {
        "root": (
            (center_x, center_y, zmin),
            (center_x, center_y, zmin + .075 * height),
            None,
        ),
        "hips": (
            (center_x, center_y, zmin + .385 * height),
            (center_x, center_y, zmin + .475 * height),
            "root",
        ),
        "spine": (
            (center_x, center_y, zmin + .475 * height),
            (center_x, center_y, zmin + .585 * height),
            "hips",
        ),
        "chest": (
            (center_x, center_y, zmin + .585 * height),
            (center_x, center_y, zmin + .700 * height),
            "spine",
        ),
        "head": (
            (center_x, center_y, zmin + .700 * height),
            (center_x, center_y, zmin + .940 * height),
            "chest",
        ),
        "upper_arm.L": (
            (center_x - shoulder * .52, center_y, zmin + .675 * height),
            (center_x - elbow, center_y, zmin + .535 * height),
            "chest",
        ),
        "forearm.L": (
            (center_x - elbow, center_y, zmin + .535 * height),
            (center_x - wrist, center_y, zmin + .405 * height),
            "upper_arm.L",
        ),
        "hand.L": (
            (center_x - wrist, center_y, zmin + .405 * height),
            (center_x - hand_tip, center_y, zmin + .340 * height),
            "forearm.L",
        ),
        "upper_arm.R": (
            (center_x + shoulder * .52, center_y, zmin + .675 * height),
            (center_x + elbow, center_y, zmin + .535 * height),
            "chest",
        ),
        "forearm.R": (
            (center_x + elbow, center_y, zmin + .535 * height),
            (center_x + wrist, center_y, zmin + .405 * height),
            "upper_arm.R",
        ),
        "hand.R": (
            (center_x + wrist, center_y, zmin + .405 * height),
            (center_x + hand_tip, center_y, zmin + .340 * height),
            "forearm.R",
        ),
        "thigh.L": (
            (center_x - .065 * height, center_y, zmin + .420 * height),
            (center_x - .078 * height, center_y, zmin + .235 * height),
            "hips",
        ),
        "shin.L": (
            (center_x - .078 * height, center_y, zmin + .235 * height),
            (center_x - .080 * height, center_y, zmin + .040 * height),
            "thigh.L",
        ),
        "thigh.R": (
            (center_x + .065 * height, center_y, zmin + .420 * height),
            (center_x + .078 * height, center_y, zmin + .235 * height),
            "hips",
        ),
        "shin.R": (
            (center_x + .078 * height, center_y, zmin + .235 * height),
            (center_x + .080 * height, center_y, zmin + .040 * height),
            "thigh.R",
        ),
    }
    right_hand_head = Vector(specs["hand.R"][0])
    right_hand_tail = Vector(specs["hand.R"][1])
    socket_head = right_hand_head.lerp(right_hand_tail, weapon_grip_ratio)
    socket_head.y += weapon_grip_depth * height
    # Keep the socket's rest axes aligned to armature/model space.  Blender's
    # glTF exporter maps a +Z edit-bone direction to the glTF +Y-up identity
    # frame.  raylib 6.0 expands animation joints into model-space currentPose
    # transforms, so this identity bind rotation makes a grip-origin weapon
    # work with an identity runtime gripOffset while still inheriting the full
    # animated right-hand delta.
    socket_tail = socket_head + Vector((0, 0, .050 * height))
    specs["weapon_socket.R"] = (
        tuple(socket_head),
        tuple(socket_tail),
        "hand.R",
    )
    return specs


def create_rig(actor, mesh):
    specs = humanoid_specs(
        mesh,
        actor["weapon_grip_ratio"],
        actor["weapon_grip_depth"],
    )
    armature = bpy.data.armatures.new(actor["id"] + "_runtime_skeleton")
    rig = bpy.data.objects.new(actor["id"] + "_runtime_rig", armature)
    bpy.context.scene.collection.objects.link(rig)
    select_only([rig])
    bpy.ops.object.mode_set(mode="EDIT")
    for name, (head, tail, parent) in specs.items():
        bone = armature.edit_bones.new(name)
        bone.head = head
        bone.tail = tail
        # Keep the socket out of Blender's Bone Heat solve.  It is marked as a
        # deform-capable joint after body weights are sanitized, which ensures
        # glTF retains it in skin.joints without giving it any body vertices.
        bone.use_deform = name not in {"root", "weapon_socket.R"}
        if parent:
            bone.parent = armature.edit_bones[parent]
    bpy.ops.object.mode_set(mode="OBJECT")
    rig.show_in_front = True
    rig.data.display_type = "BBONE"
    rig["joint_contract"] = "modular_hero_16_v1"
    rig["weapon_socket"] = "weapon_socket.R"
    rig["weapon_pivot_contract"] = "grip_at_local_origin"
    rig["weapon_grip_offset"] = "identity"
    rig["weapon_grip_ratio"] = actor["weapon_grip_ratio"]
    rig["weapon_grip_depth"] = actor["weapon_grip_depth"]
    rig["weapon_socket_bind_rotation"] = "identity_model_space"
    rig["source_axis"] = "+Z_up_-Y_forward"
    return rig, specs


def point_segment_distance(point, start, end):
    start, end = Vector(start), Vector(end)
    delta = end - start
    if delta.length_squared < 1e-9:
        return (point - start).length
    ratio = max(0.0, min(1.0, (point - start).dot(delta) / delta.length_squared))
    return (point - (start + delta * ratio)).length


def normalized_distance_weights(point, candidates, specs, sigma):
    raw = []
    for name in candidates:
        distance = point_segment_distance(point, specs[name][0], specs[name][1])
        raw.append((name, math.exp(-((distance / max(.001, sigma)) ** 2))))
    raw.sort(key=lambda item: item[1], reverse=True)
    raw = raw[:3]
    total = sum(weight for _, weight in raw)
    if total < 1e-8:
        return [(raw[0][0], 1.0)]
    kept = [(name, weight / total) for name, weight in raw if weight / total > .015]
    kept_total = sum(weight for _, weight in kept)
    if kept_total < 1e-8:
        return [(raw[0][0], 1.0)]
    return [(name, weight / kept_total) for name, weight in kept]


def smoothstep(edge0, edge1, value):
    if edge1 <= edge0:
        return 1.0 if value >= edge1 else 0.0
    ratio = max(0.0, min(1.0, (value - edge0) / (edge1 - edge0)))
    return ratio * ratio * (3.0 - 2.0 * ratio)


def blend_weight_sets(existing, semantic, strength):
    """Blend two sparse weight sets without introducing a hard region seam."""
    combined = {}
    for name, weight in existing:
        combined[name] = combined.get(name, 0.0) + weight * (1.0 - strength)
    for name, weight in semantic:
        combined[name] = combined.get(name, 0.0) + weight * strength
    ordered = sorted(combined.items(), key=lambda item: item[1], reverse=True)[:4]
    total = sum(weight for _, weight in ordered)
    if total < 1e-8:
        return semantic or existing
    return [(name, weight / total) for name, weight in ordered]


def rear_accessory_route(point, actor, bounds, specs):
    """Return smooth ponytail/cape routing strength and torso/head candidates.

    A binary y/z test tears connected triangles when one side keeps Bone Heat
    and its neighbour becomes rigidly chest weighted.  These assets are a
    single manifold, so fade the semantic routing across both depth and height.
    Deep rear vertices still receive full semantic weights and can never leak
    to an arm or a leg.
    """
    xmin, xmax, ymin, ymax, zmin, zmax = bounds
    del xmin, xmax
    height = zmax - zmin
    depth = ymax - ymin
    center_y = specs["hips"][0][1]
    normalized_z = (point.z - zmin) / max(.001, height)

    if actor["style"] == "knight":
        # The Knight's ponytail is close enough to the head/chest chain for
        # Bone Heat to solve it without crossing a limb.  A broad "rear"
        # override also catches the back-facing half of her gauntlets and was
        # the source of long arm/waist spikes in twisting poses, so do not
        # semantically reroute this actor.
        strength = 0.0
        if normalized_z > .66:
            candidates = ["head", "chest"]
        elif normalized_z > .48:
            candidates = ["chest", "head", "spine"]
        else:
            candidates = ["spine", "hips", "chest"]
        return strength, candidates

    # Keep the transition zone out of the sleeves/hands; Bone Heat is already
    # continuous there.  Only the deep rear cape receives a semantic torso
    # route.  The fade begins behind the arm silhouette and reaches full
    # strength well inside the cape volume.
    lower_cape = 1.0 - smoothstep(.36, .50, normalized_z)
    depth_start = .12 - .07 * lower_cape
    depth_full = .22 - .08 * lower_cape
    depth_strength = smoothstep(
        center_y + depth * depth_start,
        center_y + depth * depth_full,
        point.y,
    )
    height_strength = smoothstep(.08, .20, normalized_z)
    strength = depth_strength * height_strength
    if normalized_z > .73:
        candidates = ["head", "chest"]
    elif normalized_z > .56:
        candidates = ["chest", "spine", "head"]
    elif normalized_z > .34:
        candidates = ["spine", "hips", "chest"]
    else:
        candidates = ["hips", "spine"]
    return strength, candidates


def humanoid_candidates(point, actor, bounds, specs):
    xmin, xmax, ymin, ymax, zmin, zmax = bounds
    height = zmax - zmin
    center_x = specs["hips"][0][0]
    center_y = specs["hips"][0][1]
    normalized_z = (point.z - zmin) / max(.001, height)
    delta_x = point.x - center_x
    side = "L" if delta_x < 0 else "R"
    depth = ymax - ymin

    if normalized_z > .715 and abs(delta_x) < .27 * height:
        return ["head", "chest"]
    if normalized_z < .445:
        if abs(delta_x) < .025 * height and normalized_z > .34:
            return ["hips", "thigh.L", "thigh.R"]
        if normalized_z > .235:
            return [f"thigh.{side}", f"shin.{side}", "hips"]
        return [f"shin.{side}", f"thigh.{side}"]
    if abs(delta_x) > .105 * height and normalized_z < .735:
        if normalized_z > .535:
            return [f"upper_arm.{side}", f"forearm.{side}", "chest"]
        if normalized_z > .395:
            return [f"forearm.{side}", f"upper_arm.{side}", f"hand.{side}"]
        return [f"hand.{side}", f"forearm.{side}"]
    if normalized_z > .595:
        return ["chest", "spine", "head"]
    if normalized_z > .475:
        return ["spine", "chest", "hips"]
    return ["hips", "spine", "thigh.L", "thigh.R"]


def smooth_deform_weights(bm, deform_layer, allowed_indices, iterations=2, factor=.28):
    """Smooth only across real topology edges, retaining four influences.

    Hunyuan's joined character surface can place a cape and sleeve very close
    together.  Bone Heat is spatial, so adjacent vertices sometimes choose
    different chains and a tiny triangle stretches into a visible spike.  A
    short topology-aware relaxation removes that discontinuity without
    bleeding weights across merely-near surfaces.
    """
    bm.verts.ensure_lookup_table()
    for _ in range(iterations):
        snapshot = []
        for vertex in bm.verts:
            snapshot.append({
                index: weight
                for index, weight in vertex[deform_layer].items()
                if index in allowed_indices and weight > 1e-7
            })
        updates = []
        for vertex in bm.verts:
            neighbours = [edge.other_vert(vertex).index for edge in vertex.link_edges]
            if not neighbours:
                updates.append(snapshot[vertex.index])
                continue
            averaged = {}
            for neighbour_index in neighbours:
                for group_index, weight in snapshot[neighbour_index].items():
                    averaged[group_index] = averaged.get(group_index, 0.0) + weight
            neighbour_scale = factor / len(neighbours)
            blended = {
                group_index: weight * (1.0 - factor)
                for group_index, weight in snapshot[vertex.index].items()
            }
            for group_index, weight in averaged.items():
                blended[group_index] = blended.get(group_index, 0.0) + weight * neighbour_scale
            ordered = sorted(blended.items(), key=lambda item: item[1], reverse=True)[:4]
            total = sum(weight for _, weight in ordered)
            updates.append({
                group_index: weight / max(total, 1e-8)
                for group_index, weight in ordered
            })
        for vertex, weights in zip(bm.verts, updates):
            deform_vertex = vertex[deform_layer]
            deform_vertex.clear()
            for group_index, weight in weights.items():
                deform_vertex[group_index] = weight


def skin_body(actor, mesh, rig, specs):
    bone_heat_error = None
    try:
        select_only([mesh, rig], active=rig)
        bpy.ops.object.parent_set(type="ARMATURE_AUTO")
    except RuntimeError as error:
        bone_heat_error = str(error)
        print(
            f"{actor['id']}: Bone Heat did not fully solve; deterministic weights will fill gaps: {error}",
            flush=True,
        )

    modifier = next((item for item in mesh.modifiers if item.type == "ARMATURE"), None)
    if modifier is None:
        modifier = mesh.modifiers.new("Runtime Armature", "ARMATURE")
        modifier.object = rig
        mesh.parent = rig
        mesh.matrix_parent_inverse = rig.matrix_world.inverted()
    else:
        modifier.name = "Runtime Armature"
        modifier.object = rig

    groups = {
        name: mesh.vertex_groups.get(name) or mesh.vertex_groups.new(name=name)
        for name in BODY_BONES
    }
    allowed_indices = {group.index for group in groups.values()}
    index_to_name = {group.index: name for name, group in groups.items()}
    bounds = mesh_bounds(mesh)
    height = bounds[5] - bounds[4]
    sigma = height * .10

    bm = bmesh.new()
    bm.from_mesh(mesh.data)
    bm.verts.ensure_lookup_table()
    deform_layer = bm.verts.layers.deform.verify()
    fallback_vertices = 0
    semantic_vertices = 0
    for vertex in bm.verts:
        deform_vertex = vertex[deform_layer]
        existing = [
            (index_to_name[index], weight)
            for index, weight in deform_vertex.items()
            if index in allowed_indices and weight > 1e-6
        ]
        candidates = humanoid_candidates(vertex.co, actor, bounds, specs)
        if not existing:
            existing = normalized_distance_weights(vertex.co, candidates, specs, sigma)
            fallback_vertices += 1
        else:
            existing.sort(key=lambda item: item[1], reverse=True)
            existing = existing[:4]
            total = sum(weight for _, weight in existing)
            existing = [(name, weight / max(total, 1e-8)) for name, weight in existing]

        # The connected rear ponytail/cape volumes need semantic routing, but
        # use a smooth blend so adjacent triangles never jump from a leg/arm to
        # a rigid head/chest assignment at a single coordinate threshold.
        route_strength, route_candidates = rear_accessory_route(
            vertex.co, actor, bounds, specs
        )
        if route_strength > 1e-5:
            semantic = normalized_distance_weights(
                vertex.co, route_candidates, specs, sigma
            )
            existing = blend_weight_sets(existing, semantic, route_strength)
            semantic_vertices += 1

        deform_vertex.clear()
        total = sum(weight for _, weight in existing)
        for name, weight in existing:
            deform_vertex[groups[name].index] = weight / max(total, 1e-8)

    smooth_deform_weights(
        bm,
        deform_layer,
        allowed_indices,
        iterations=2 if actor["style"] == "caster" else 0,
        factor=.28,
    )

    bm.to_mesh(mesh.data)
    bm.free()

    socket_group = mesh.vertex_groups.get("weapon_socket.R")
    if socket_group:
        mesh.vertex_groups.remove(socket_group)
    root_group = mesh.vertex_groups.get("root")
    if root_group:
        mesh.vertex_groups.remove(root_group)

    # Export the socket as a skin joint after the body-only weight solve.
    rig.data.bones["weapon_socket.R"].use_deform = True
    modifier.use_deform_preserve_volume = True
    mesh.data.validate(clean_customdata=False)
    mesh.data.update()
    return {
        "boneHeatError": bone_heat_error,
        "deterministicVertices": fallback_vertices,
        "rearAccessoryBlendedVertices": semantic_vertices,
    }


def audit_weights(mesh):
    allowed = {mesh.vertex_groups[name].index for name in BODY_BONES}
    forbidden = {
        group.index
        for group in mesh.vertex_groups
        if group.name in {"root", "weapon_socket.R"}
    }
    unweighted = 0
    forbidden_weighted = 0
    maximum = 0
    minimum_sum = 1.0
    for vertex in mesh.data.vertices:
        influences = [
            item.weight for item in vertex.groups
            if item.group in allowed and item.weight > 1e-6
        ]
        if any(
            item.group in forbidden and item.weight > 1e-6
            for item in vertex.groups
        ):
            forbidden_weighted += 1
        if not influences:
            unweighted += 1
            minimum_sum = 0.0
        else:
            maximum = max(maximum, len(influences))
            minimum_sum = min(minimum_sum, sum(influences))
    return {
        "unweightedVertices": unweighted,
        "forbiddenSocketWeightedVertices": forbidden_weighted,
        "maximumInfluences": maximum,
        "minimumWeightSum": round(minimum_sum, 6),
    }


def validate_rig(actor, rig):
    names = [bone.name for bone in rig.data.bones]
    if set(names) != set(EXPECTED_JOINTS) or len(names) != 16:
        raise RuntimeError(f"Expected exact 16-joint contract; got {names}")
    socket = rig.data.bones["weapon_socket.R"]
    if socket.parent is None or socket.parent.name != "hand.R":
        raise RuntimeError("weapon_socket.R must be a direct child of hand.R")
    hand = rig.data.bones["hand.R"]
    expected_head = hand.head_local.lerp(
        hand.tail_local, actor["weapon_grip_ratio"]
    )
    body_height = (
        rig.data.bones["root"].tail_local
        - rig.data.bones["root"].head_local
    ).length / .075
    expected_head.y += actor["weapon_grip_depth"] * body_height
    if (socket.head_local - expected_head).length > 1e-5:
        raise RuntimeError("weapon_socket.R head must sit at the right-palm grip point")
    socket_direction = (socket.tail_local - socket.head_local).normalized()
    if socket_direction.dot(Vector((0, 0, 1))) < .999:
        raise RuntimeError(
            "weapon_socket.R must use the model-space identity bind axis (+Z in Blender)"
        )


def reset_pose(rig):
    for bone in rig.pose.bones:
        bone.rotation_mode = "XYZ"
        bone.location = (0, 0, 0)
        bone.rotation_euler = (0, 0, 0)
        bone.scale = (1, 1, 1)


def key_pose(rig, frame, values=None):
    reset_pose(rig)
    for key, value in (values or {}).items():
        bone_name, channel = key.rsplit(".", 1)
        bone = rig.pose.bones[bone_name]
        if channel == "loc":
            bone.location = value
        elif channel == "rot":
            bone.rotation_euler = value
        elif channel == "scale":
            bone.scale = value
        else:
            raise RuntimeError(f"Unknown pose channel: {key}")
    for bone in rig.pose.bones:
        if bone.name == "weapon_socket.R":
            # The modular weapon curve is authored in one dedicated pass.
            # Body keys on this joint introduced identity keys between combat
            # poses, causing one-frame weapon pops in the runtime sampler.
            continue
        bone.keyframe_insert("location", frame=frame, group=bone.name)
        bone.keyframe_insert("rotation_euler", frame=frame, group=bone.name)
        bone.keyframe_insert("scale", frame=frame, group=bone.name)


def key_weapon_socket(rig, frame, rotation):
    """Author a modular weapon pose without touching the body animation."""
    socket = rig.pose.bones["weapon_socket.R"]
    socket.rotation_mode = "XYZ"
    socket.rotation_euler = rotation
    socket.keyframe_insert("rotation_euler", frame=frame, group=socket.name)


def magic_caster_weapon_animation(rig):
    # Blender edit bones point along local Y even though this socket's tail is
    # authored along model +Z.  A -90-degree local X correction maps the
    # staff's local +Z crystal end back to world up.  Local Z then provides the
    # readable side tilt used by the individual attack poses.
    # The caster grip sits deeper in the palm than the Knight grip.  A slightly
    # stronger resting lean keeps the lower end of the long staff above ground.
    rest = -.56
    poses = {
        0: rest, 24: rest, 47: rest, 71: rest, 94: rest,
        96: rest, 104: rest, 112: rest, 120: rest, 127: rest,
        135: rest, 143: rest, 151: rest, 158: rest,
        # Basic projectile: draw inward, spear the crystal toward the target.
        160: rest, 168: .24, 188: 2.12,
        206: -.48, 222: rest,
        # Frost nova: staff rises with the charge and snaps outward.
        224: rest, 242: .38, 260: .72, 274: 2.22,
        290: -.48, 302: rest,
        # Gravity snare: a wide staff orbit mirrors the torso twist.
        304: rest, 324: .92, 344: -.45,
        364: -.52, 382: rest,
        # Ultimate: overhead channel into the strongest downward release.
        384: rest, 408: .56, 432: .98, 448: 2.20,
        466: -.58, 478: rest,
        # Blink thrust.
        480: rest, 492: .18, 505: -.98,
        518: -.44, 526: rest,
        528: rest, 540: .18, 550: .06, 558: rest,
        560: rest, 584: rest, 612: rest, 638: rest, 654: rest,
    }
    for frame, tilt in poses.items():
        key_weapon_socket(rig, frame, (-math.pi * .5, 0, tilt))


def magic_caster_run_staff_animation(rig, height):
    """Keep the staff visibly gripped in idle and clear during locomotion.

    The generic Euler arm swing is strong enough to read in gameplay, but its
    inherited hand roll can flip a long two-ended staff through the front leg.
    Bake the staff arm from wrist targets and keep the shaft almost vertical in
    armature space.  The free arm retains the full authored counter-swing.
    """
    right_pole = Vector((.44 * height, -.02 * height, .56 * height))
    staff_direction = Vector((-.14, .02, .99)).normalized()
    scene = bpy.context.scene
    for frame, breath in [(0, 0), (24, 1), (47, 0), (71, -1), (94, 0)]:
        scene.frame_set(frame)
        bpy.context.view_layer.update()
        place_two_bone_arm(
            rig,
            "R",
            Vector((
                .20 * height,
                -.045 * height,
                (.455 + .004 * breath) * height,
            )),
            right_pole,
            frame,
        )
        orient_weapon_socket(rig, staff_direction, frame)

    run_frames = [96, 104, 112, 120, 127, 135, 143, 151, 158]
    for index, frame in enumerate(run_frames):
        phase = index / (len(run_frames) - 1) * math.tau * 2
        swing = math.sin(phase)
        lift = abs(swing)
        scene.frame_set(frame)
        bpy.context.view_layer.update()
        place_two_bone_arm(
            rig,
            "R",
            Vector((
                .20 * height,
                (-.04 - .055 * swing) * height,
                (.465 + .010 * lift) * height,
            )),
            right_pole,
            frame,
        )
        orient_weapon_socket(rig, staff_direction, frame)


def knight_weapon_animation(rig):
    # Keep the 1.55 m greatsword clear of the floor during locomotion, then
    # rotate its blade into the camera plane at attack payoffs.  The previous
    # identity socket aimed the blade almost straight through camera depth,
    # making a full-size greatsword read like a dagger.
    rest = (0, -1.10, 0)
    poses = {
        0: rest, 24: rest, 47: rest, 71: rest, 94: rest,
        96: rest, 104: rest, 112: rest, 120: rest, 127: rest,
        135: rest, 143: rest, 151: rest, 158: rest,
        # Basic sword arc.
        160: rest, 168: (0, -2.80, 0), 188: (0, -1.20, 0),
        206: (0, -1.28, 0), 222: rest,
        # Aegis guard keeps the blade upright across the chest before the shove.
        224: rest, 242: (0, 3.06, 0), 274: (0, -.72, 0),
        290: (0, -1.00, 0), 302: rest,
        # Ground slam.
        304: rest, 324: (0, -2.16, 0), 344: (0, -1.40, 0),
        364: (0, -1.26, 0), 382: rest,
        # Ultimate charge and release.
        384: rest, 408: (0, -2.90, 0), 432: (0, -2.22, 0),
        448: (0, 1.58, 0),
        466: (0, -1.24, 0), 478: rest,
        # Dash lunge presents the blade as one long forward line.
        480: rest, 492: (0, 2.84, 0), 505: (0, -1.48, 0),
        518: (0, -1.22, 0), 526: rest,
        528: rest, 540: (0, -.92, 0), 550: rest, 558: rest,
        560: rest, 584: rest, 612: rest, 638: rest, 654: rest,
    }
    for frame, rotation in poses.items():
        key_weapon_socket(rig, frame, rotation)


def humanoid_animation(actor, rig, height):
    style = actor["style"]
    for frame, phase in [(0, 0), (24, 1), (47, 2), (71, 3), (94, 4)]:
        breath = math.sin(phase * math.pi * .5)
        idle_arm = .065 if style == "knight" else .055
        idle_forearm = .045 if style == "knight" else .060
        key_pose(rig, frame, {
            "hips.loc": (0, 0, abs(breath) * height * .006),
            "chest.rot": (.018 * breath, 0, .010 * breath),
            "head.rot": (-.010 * breath, 0, -.008 * breath),
            "upper_arm.L.rot": (-idle_arm * breath, 0, .026 * breath),
            "upper_arm.R.rot": (idle_arm * .82 * breath, 0, -.022 * breath),
            "forearm.L.rot": (-idle_forearm * abs(breath), 0, .016 * breath),
            "forearm.R.rot": (-idle_forearm * .78 * abs(breath), 0, -.014 * breath),
            "hand.L.rot": (.030 * breath, -.012 * breath, -.026 * breath),
            "hand.R.rot": (-.024 * breath, .010 * breath, .022 * breath),
        })

    run_frames = [96, 104, 112, 120, 127, 135, 143, 151, 158]
    run_amplitude = .20 if style == "knight" else .62
    for index, frame in enumerate(run_frames):
        phase = index / (len(run_frames) - 1) * math.tau * 2
        swing = math.sin(phase)
        lift = abs(math.sin(phase))
        arm_left = -run_amplitude * .78 * swing
        arm_right = run_amplitude * .78 * swing
        shin_bend = .48
        forearm_left = -.26 * lift
        forearm_right = -.16 * lift
        hand_left = (-.14 * swing, .035 * swing, .075 * swing)
        hand_right = (.09 * swing, -.025 * swing, -.050 * swing)
        chest_yaw = -.095 * swing
        if style == "knight":
            # The greatsword is carried rather than flung like a light prop,
            # while the free arm drives a strong, readable counter-swing.
            # These peaks remain below the shoulder/thigh clipping threshold.
            arm_left = -.44 * swing
            arm_right = .31 * swing
            shin_bend = .18
            forearm_left = -.23 * lift
            forearm_right = -.15 * lift
            hand_left = (.17 * swing, .030 * swing, .085 * swing)
            hand_right = (-.115 * swing, -.020 * swing, -.055 * swing)
            chest_yaw = -.11 * swing
        else:
            # The staff hand stays calmer than the free hand, but both arms now
            # carry enough shoulder/elbow/wrist motion to read from game camera.
            arm_left = -.68 * swing
            arm_right = .53 * swing
            forearm_left = -.34 * lift
            forearm_right = -.23 * lift
            hand_left = (-.20 * swing, .045 * swing, .10 * swing)
            hand_right = (.12 * swing, -.030 * swing, -.065 * swing)
        key_pose(rig, frame, {
            "hips.loc": (0, 0, lift * height * (.003 if style == "knight" else .012)),
            "hips.rot": (.035 * math.sin(phase * 2), 0, .025 * swing),
            "chest.rot": (.10, 0, chest_yaw),
            "head.rot": (-.045, 0, .025 * swing),
            "thigh.L.rot": (run_amplitude * swing, 0, 0),
            "thigh.R.rot": (-run_amplitude * swing, 0, 0),
            "shin.L.rot": (-shin_bend * max(0, -swing), 0, 0),
            "shin.R.rot": (-shin_bend * max(0, swing), 0, 0),
            "upper_arm.L.rot": (arm_left, 0, 0),
            "upper_arm.R.rot": (arm_right, 0, 0),
            "forearm.L.rot": (forearm_left, 0, .035 * swing),
            "forearm.R.rot": (forearm_right, 0, -.030 * swing),
            "hand.L.rot": hand_left,
            "hand.R.rot": hand_right,
        })

    key_pose(rig, 160)
    if style == "knight":
        key_pose(rig, 174, {
            "chest.rot": (0, 0, -.18),
            "upper_arm.R.rot": (-.08, 0, -.08),
            "forearm.R.rot": (-.04, 0, 0),
            "thigh.R.rot": (-.10, 0, 0),
        })
        impact = {
            "hips.loc": (0, -.015 * height, 0),
            "chest.rot": (.05, 0, .22),
            "upper_arm.R.rot": (.04, 0, .12),
            "forearm.R.rot": (-.03, 0, .05),
            "upper_arm.L.rot": (-.10, 0, -.14),
            "head.rot": (-.03, 0, -.10),
        }
        key_pose(rig, 187, impact)
        key_pose(rig, 192, impact)
        key_pose(rig, 207, {
            "chest.rot": (.03, 0, .10),
            "upper_arm.R.rot": (.02, 0, .04),
        })
    else:
        key_pose(rig, 174, {
            "chest.rot": (-.08, 0, -.12),
            "upper_arm.L.rot": (-.72, 0, -.48),
            "forearm.L.rot": (-.46, 0, -.25),
            "upper_arm.R.rot": (-.48, 0, .40),
        })
        key_pose(rig, 188, {
            "chest.rot": (.12, 0, .18),
            "upper_arm.L.rot": (.42, 0, -.72),
            "forearm.L.rot": (-.20, 0, -.45),
            "upper_arm.R.rot": (.30, 0, .70),
            "hips.loc": (0, -.02 * height, .01 * height),
        })
        key_pose(rig, 206, {
            "chest.rot": (.04, 0, .06),
            "upper_arm.L.rot": (.10, 0, -.22),
            "upper_arm.R.rot": (.08, 0, .20),
        })
    key_pose(rig, 222)

    key_pose(rig, 224)
    key_pose(rig, 242, {
        "hips.loc": (0, 0, -.012 * height),
        "chest.rot": (-.14, 0, 0),
        "upper_arm.L.rot": (-.88, 0, -.42),
        "upper_arm.R.rot": (-.88, 0, .42),
        "forearm.L.rot": (-.52, 0, 0),
        "forearm.R.rot": (-.52, 0, 0),
    })
    key_pose(rig, 260, {
        "hips.loc": (0, 0, .012 * height),
        "chest.rot": (-.05, 0, .08),
        "upper_arm.L.rot": (-1.08, 0, -.28),
        "upper_arm.R.rot": (-1.08, 0, .28),
    })
    key_pose(rig, 274, {
        "hips.loc": (0, -.035 * height, 0),
        "chest.rot": (.26, 0, 0),
        "upper_arm.L.rot": (.65, 0, -.18),
        "upper_arm.R.rot": (.65, 0, .18),
        "forearm.L.rot": (-.20, 0, 0),
        "forearm.R.rot": (-.20, 0, 0),
    })
    key_pose(rig, 290, {
        "chest.rot": (.08, 0, 0),
        "upper_arm.L.rot": (.18, 0, 0),
        "upper_arm.R.rot": (.18, 0, 0),
    })
    key_pose(rig, 302)

    key_pose(rig, 304)
    key_pose(rig, 324, {
        "chest.rot": (0, 0, -.62),
        "hips.rot": (0, 0, -.20),
        "upper_arm.L.rot": (0, 0, -.82),
        "upper_arm.R.rot": (0, 0, -.76),
    })
    key_pose(rig, 344, {
        "chest.rot": (.04, 0, .76),
        "hips.rot": (0, 0, .26),
        "upper_arm.L.rot": (0, 0, 1.02),
        "upper_arm.R.rot": (0, 0, .92),
        "hips.loc": (0, -.02 * height, 0),
    })
    key_pose(rig, 364, {
        "chest.rot": (0, 0, .20),
        "upper_arm.L.rot": (0, 0, .24),
        "upper_arm.R.rot": (0, 0, .22),
    })
    key_pose(rig, 382)

    key_pose(rig, 384)
    key_pose(rig, 408, {
        "hips.loc": (0, 0, -.018 * height),
        "chest.rot": (-.20, 0, 0),
        "upper_arm.L.rot": (-1.10, 0, -.55),
        "upper_arm.R.rot": (-1.10, 0, .55),
        "forearm.L.rot": (-.62, 0, 0),
        "forearm.R.rot": (-.62, 0, 0),
    })
    key_pose(rig, 432, {
        "hips.loc": (0, 0, .025 * height),
        "chest.rot": (-.08, 0, 0),
        "upper_arm.L.rot": (-1.34, 0, -.24),
        "upper_arm.R.rot": (-1.34, 0, .24),
        "head.rot": (-.14, 0, 0),
    })
    key_pose(rig, 448, {
        "hips.loc": (0, -.055 * height, 0),
        "chest.rot": (.38, 0, 0),
        "upper_arm.L.rot": (.84, 0, -.20),
        "upper_arm.R.rot": (.84, 0, .20),
        "forearm.L.rot": (-.12, 0, 0),
        "forearm.R.rot": (-.12, 0, 0),
    })
    key_pose(rig, 466, {
        "chest.rot": (.10, 0, 0),
        "upper_arm.L.rot": (.20, 0, 0),
        "upper_arm.R.rot": (.20, 0, 0),
    })
    key_pose(rig, 478)

    key_pose(rig, 480)
    key_pose(rig, 492, {
        "chest.rot": (-.08, 0, 0),
        "thigh.L.rot": (-.28, 0, 0),
        "thigh.R.rot": (.20, 0, 0),
        "upper_arm.L.rot": (.24, 0, -.18),
        "upper_arm.R.rot": (.24, 0, .18),
    })
    key_pose(rig, 505, {
        "hips.loc": (0, -.045 * height, .008 * height),
        "chest.rot": (.38, 0, 0),
        "head.rot": (-.18, 0, 0),
        "upper_arm.L.rot": (.56, 0, -.20),
        "upper_arm.R.rot": (.56, 0, .20),
        "thigh.L.rot": (.45, 0, 0),
        "thigh.R.rot": (-.45, 0, 0),
    })
    key_pose(rig, 518, {
        "chest.rot": (.12, 0, 0),
        "thigh.L.rot": (.12, 0, 0),
        "thigh.R.rot": (-.12, 0, 0),
    })
    key_pose(rig, 526)

    key_pose(rig, 528)
    key_pose(rig, 540, {
        "chest.rot": (-.26, 0, .24),
        "head.rot": (-.18, 0, -.16),
        "upper_arm.L.rot": (-.28, 0, -.25),
        "upper_arm.R.rot": (-.28, 0, .25),
        "hips.loc": (0, .025 * height, 0),
    })
    key_pose(rig, 550, {"chest.rot": (-.06, 0, .06)})
    key_pose(rig, 558)

    key_pose(rig, 560)
    key_pose(rig, 584, {
        "root.rot": (0, .22, 0),
        "chest.rot": (-.30, 0, -.16),
        "thigh.L.rot": (.20, 0, 0),
        "thigh.R.rot": (-.16, 0, 0),
    })
    key_pose(rig, 612, {
        "root.rot": (0, 1.08, 0),
        "root.loc": (-.12 * height, 0, -.12 * height),
        "chest.rot": (-.15, 0, 0),
        "head.rot": (.22, 0, 0),
    })
    key_pose(rig, 638, {
        "root.rot": (0, 1.48, 0),
        "root.loc": (-.22 * height, 0, -.28 * height),
        "upper_arm.L.rot": (.20, 0, -.40),
        "upper_arm.R.rot": (.20, 0, .40),
    })
    key_pose(rig, 654, {
        "root.rot": (0, 1.48, 0),
        "root.loc": (-.22 * height, 0, -.28 * height),
        "upper_arm.L.rot": (.20, 0, -.40),
        "upper_arm.R.rot": (.20, 0, .40),
    })


def knight_combat_animation(rig, height):
    """Author readable one-handed greatsword combat for the Knight.

    The previous master action mostly rotated the weapon socket while leaving
    the shoulders close to the bind pose.  These poses drive the action from
    feet -> hips -> chest -> shoulder -> hand.  Contact poses hold for only two
    frames; the runtime owns any longer hit-stop or time scaling.
    """
    # Basic -- broad right-to-left slash with an obvious recoil and follow-through.
    key_pose(rig, 160)
    key_pose(rig, 168, {
        "hips.rot": (0, 0, -.22),
        "hips.loc": (0, .012 * height, -.012 * height),
        "chest.rot": (-.10, -.08, -.52),
        "head.rot": (.04, 0, .20),
        "upper_arm.R.rot": (-.62, .08, -.72),
        "forearm.R.rot": (-.62, -.08, -.22),
        "upper_arm.L.rot": (-.18, 0, -.24),
        "forearm.L.rot": (-.30, 0, -.12),
        "thigh.L.rot": (.10, 0, 0),
        "thigh.R.rot": (-.12, 0, 0),
    })
    basic_contact = {
        "hips.rot": (0, 0, .30),
        "hips.loc": (0, -.055 * height, -.006 * height),
        "chest.rot": (.08, .03, .42),
        "head.rot": (-.08, 0, -.28),
        "upper_arm.R.rot": (-.38, -.04, 1.02),
        "forearm.R.rot": (-.12, 0, .34),
        "upper_arm.L.rot": (-.34, 0, .30),
        "forearm.L.rot": (-.22, 0, .18),
        "thigh.L.rot": (-.16, 0, 0),
        "thigh.R.rot": (.20, 0, 0),
    }
    key_pose(rig, 188, basic_contact)
    key_pose(rig, 206, {
        "hips.rot": (0, 0, .12),
        "chest.rot": (.06, 0, .28),
        "head.rot": (-.03, 0, -.10),
        "upper_arm.R.rot": (.16, 0, .34),
        "forearm.R.rot": (-.08, 0, .12),
        "upper_arm.L.rot": (-.10, 0, .10),
    })
    key_pose(rig, 222)

    # Skill One -- Aegis guard: plant the feet, catch the hit, then shove out.
    key_pose(rig, 224)
    key_pose(rig, 242, {
        "hips.loc": (0, .018 * height, -.040 * height),
        "hips.rot": (.04, 0, -.10),
        "chest.rot": (-.16, 0, -.18),
        "head.rot": (.08, 0, .06),
        "upper_arm.R.rot": (-.76, .12, -.36),
        "forearm.R.rot": (-.92, 0, -.28),
        "upper_arm.L.rot": (-.72, -.08, .30),
        "forearm.L.rot": (-.82, 0, .22),
        "thigh.L.rot": (.26, 0, -.08),
        "thigh.R.rot": (.26, 0, .08),
        "shin.L.rot": (-.36, 0, 0),
        "shin.R.rot": (-.36, 0, 0),
    })
    guard_contact = {
        "hips.loc": (0, -.030 * height, -.030 * height),
        "chest.rot": (.24, 0, .12),
        "head.rot": (-.12, 0, -.04),
        "upper_arm.R.rot": (-.18, 0, .22),
        "forearm.R.rot": (-.40, 0, .16),
        "upper_arm.L.rot": (-.24, 0, -.22),
        "forearm.L.rot": (-.34, 0, -.18),
        "thigh.L.rot": (.16, 0, 0),
        "thigh.R.rot": (.16, 0, 0),
        "shin.L.rot": (-.22, 0, 0),
        "shin.R.rot": (-.22, 0, 0),
    }
    key_pose(rig, 274, guard_contact)
    key_pose(rig, 290, {
        "chest.rot": (.08, 0, .05),
        "upper_arm.R.rot": (-.12, 0, .08),
        "forearm.R.rot": (-.16, 0, .06),
        "upper_arm.L.rot": (-.08, 0, -.06),
    })
    key_pose(rig, 302)

    # Skill Two -- shield-rush body mechanics: low stance, wide stride, sword
    # tucked beside the body so the leading shoulder owns the silhouette.
    key_pose(rig, 304)
    key_pose(rig, 324, {
        "hips.loc": (0, .010 * height, 0),
        "chest.rot": (-.10, -.16, 0),
        "head.rot": (.06, .08, 0),
        "upper_arm.R.rot": (-.70, .10, -.34),
        "forearm.R.rot": (-.76, -.06, -.18),
        "upper_arm.L.rot": (-.54, 0, -.34),
        "forearm.L.rot": (-.62, 0, -.18),
        "thigh.L.rot": (-.24, 0, 0),
        "thigh.R.rot": (.24, 0, 0),
        "shin.L.rot": (-.14, 0, 0),
        "shin.R.rot": (-.34, 0, 0),
    })
    slam_contact = {
        "hips.loc": (0, -.055 * height, 0),
        "chest.rot": (.24, .18, 0),
        "head.rot": (-.12, -.08, 0),
        "upper_arm.R.rot": (-.48, -.04, .58),
        "forearm.R.rot": (-.18, 0, .18),
        "upper_arm.L.rot": (-.62, 0, -.62),
        "forearm.L.rot": (-.22, 0, -.30),
        "thigh.L.rot": (.46, 0, 0),
        "thigh.R.rot": (-.42, 0, 0),
        "shin.L.rot": (-.26, 0, 0),
        "shin.R.rot": (-.12, 0, 0),
    }
    key_pose(rig, 344, slam_contact)
    key_pose(rig, 364, {
        "hips.loc": (0, -.015 * height, -.025 * height),
        "chest.rot": (.22, 0, .08),
        "head.rot": (-.10, 0, 0),
        "upper_arm.R.rot": (.28, 0, .12),
        "forearm.R.rot": (.06, 0, 0),
        "thigh.L.rot": (.14, 0, 0),
        "thigh.R.rot": (.14, 0, 0),
        "shin.L.rot": (-.20, 0, 0),
        "shin.R.rot": (-.20, 0, 0),
    })
    key_pose(rig, 382)

    # Ultimate -- charge low, raise the blade, then release a full-body arc.
    key_pose(rig, 384)
    key_pose(rig, 408, {
        "hips.loc": (0, .020 * height, -.050 * height),
        "hips.rot": (0, 0, -.30),
        "chest.rot": (-.26, -.08, -.58),
        "head.rot": (.14, 0, .24),
        "upper_arm.R.rot": (-.94, .12, -.82),
        "forearm.R.rot": (-1.04, 0, -.34),
        "upper_arm.L.rot": (-.54, 0, -.36),
        "forearm.L.rot": (-.62, 0, -.18),
        "thigh.L.rot": (.32, 0, 0),
        "thigh.R.rot": (.32, 0, 0),
        "shin.L.rot": (-.42, 0, 0),
        "shin.R.rot": (-.42, 0, 0),
    })
    key_pose(rig, 432, {
        "hips.loc": (0, .005 * height, -.015 * height),
        "hips.rot": (0, 0, -.12),
        "chest.rot": (-.42, 0, -.20),
        "head.rot": (.22, 0, .08),
        "upper_arm.R.rot": (-1.20, .10, -.46),
        "forearm.R.rot": (-1.08, 0, -.20),
        "upper_arm.L.rot": (-.82, 0, -.20),
        "forearm.L.rot": (-.78, 0, -.10),
    })
    ultimate_contact = {
        "hips.loc": (0, -.045 * height, 0),
        "chest.rot": (.28, .14, 0),
        "head.rot": (-.16, -.06, 0),
        "upper_arm.R.rot": (-.58, -.06, .66),
        "forearm.R.rot": (-.10, 0, .24),
        "upper_arm.L.rot": (-.46, 0, -.48),
        "forearm.L.rot": (-.24, 0, -.20),
        "thigh.L.rot": (.26, 0, 0),
        "thigh.R.rot": (.26, 0, 0),
        "shin.L.rot": (-.34, 0, 0),
        "shin.R.rot": (-.34, 0, 0),
    }
    key_pose(rig, 448, ultimate_contact)
    key_pose(rig, 466, {
        "hips.rot": (0, 0, .16),
        "chest.rot": (.12, 0, .36),
        "upper_arm.R.rot": (.24, 0, .44),
        "forearm.R.rot": (-.10, 0, .12),
        "upper_arm.L.rot": (-.10, 0, .16),
    })
    key_pose(rig, 478)

    # Dash special -- anticipation in the rear leg and a long piercing silhouette.
    key_pose(rig, 480)
    key_pose(rig, 492, {
        "hips.loc": (0, .025 * height, -.028 * height),
        "chest.rot": (-.22, 0, -.18),
        "head.rot": (.10, 0, .08),
        "upper_arm.R.rot": (-.48, 0, -.34),
        "forearm.R.rot": (-.60, 0, -.18),
        "thigh.L.rot": (-.32, 0, 0),
        "thigh.R.rot": (.30, 0, 0),
        "shin.L.rot": (-.18, 0, 0),
        "shin.R.rot": (-.40, 0, 0),
    })
    dash_contact = {
        "hips.loc": (0, -.095 * height, -.025 * height),
        "chest.rot": (.46, 0, .10),
        "head.rot": (-.24, 0, -.05),
        "upper_arm.R.rot": (-.42, 0, .08),
        "forearm.R.rot": (.12, 0, .02),
        "upper_arm.L.rot": (-.34, 0, -.26),
        "forearm.L.rot": (-.18, 0, -.10),
        "thigh.L.rot": (.48, 0, 0),
        "thigh.R.rot": (-.48, 0, 0),
        "shin.L.rot": (-.28, 0, 0),
        "shin.R.rot": (-.12, 0, 0),
    }
    key_pose(rig, 505, dash_contact)
    key_pose(rig, 518, {
        "hips.loc": (0, -.020 * height, -.008 * height),
        "chest.rot": (.14, 0, .03),
        "upper_arm.R.rot": (.12, 0, .02),
        "thigh.L.rot": (.12, 0, 0),
        "thigh.R.rot": (-.12, 0, 0),
    })
    key_pose(rig, 526)


def magic_caster_combat_animation(rig, height):
    """Author spell-specific full-body casts for the Magic Caster."""
    # Basic -- gather at the sternum, point with the free hand, recoil on release.
    key_pose(rig, 160)
    key_pose(rig, 168, {
        "hips.rot": (0, 0, -.12),
        "chest.rot": (-.16, -.04, -.26),
        "head.rot": (.08, 0, .12),
        "upper_arm.L.rot": (-.84, .04, -.58),
        "forearm.L.rot": (-.92, 0, -.32),
        "upper_arm.R.rot": (-.52, 0, .38),
        "forearm.R.rot": (-.64, 0, .24),
        "thigh.L.rot": (.10, 0, 0),
        "thigh.R.rot": (-.08, 0, 0),
    })
    basic_contact = {
        "hips.loc": (0, -.035 * height, .006 * height),
        "hips.rot": (0, 0, .14),
        "chest.rot": (.12, .04, .18),
        "head.rot": (-.14, 0, -.18),
        "upper_arm.L.rot": (-.52, -.04, -1.04),
        "forearm.L.rot": (-.24, 0, -.36),
        "upper_arm.R.rot": (-.42, 0, .62),
        "forearm.R.rot": (-.22, 0, .28),
        "thigh.L.rot": (-.10, 0, 0),
        "thigh.R.rot": (.12, 0, 0),
    }
    key_pose(rig, 188, basic_contact)
    key_pose(rig, 206, {
        "chest.rot": (.08, 0, .12),
        "head.rot": (-.04, 0, -.04),
        "upper_arm.L.rot": (.18, 0, -.24),
        "forearm.L.rot": (-.14, 0, -.14),
        "upper_arm.R.rot": (.10, 0, .22),
    })
    key_pose(rig, 222)

    # Skill One -- frost nova: compress, raise both hands, snap the palms outward.
    key_pose(rig, 224)
    key_pose(rig, 242, {
        "hips.loc": (0, .010 * height, -.035 * height),
        "chest.rot": (-.28, 0, 0),
        "head.rot": (.16, 0, 0),
        "upper_arm.L.rot": (-1.08, .08, -.48),
        "forearm.L.rot": (-.98, 0, -.30),
        "upper_arm.R.rot": (-1.08, -.08, .48),
        "forearm.R.rot": (-.98, 0, .30),
        "thigh.L.rot": (.18, 0, 0),
        "thigh.R.rot": (.18, 0, 0),
        "shin.L.rot": (-.28, 0, 0),
        "shin.R.rot": (-.28, 0, 0),
    })
    key_pose(rig, 260, {
        "hips.loc": (0, 0, .010 * height),
        "chest.rot": (-.18, 0, 0),
        "head.rot": (.10, 0, 0),
        "upper_arm.L.rot": (-1.24, 0, -.22),
        "forearm.L.rot": (-1.02, 0, -.12),
        "upper_arm.R.rot": (-1.24, 0, .22),
        "forearm.R.rot": (-1.02, 0, .12),
    })
    nova_contact = {
        "hips.loc": (0, -.025 * height, -.018 * height),
        "chest.rot": (.14, 0, 0),
        "head.rot": (-.08, 0, 0),
        "upper_arm.L.rot": (-.56, 0, -1.00),
        "forearm.L.rot": (-.18, 0, -.30),
        "upper_arm.R.rot": (-.42, 0, .46),
        "forearm.R.rot": (-.60, 0, .22),
        "thigh.L.rot": (.14, 0, 0),
        "thigh.R.rot": (.14, 0, 0),
        "shin.L.rot": (-.18, 0, 0),
        "shin.R.rot": (-.18, 0, 0),
    }
    key_pose(rig, 274, nova_contact)
    key_pose(rig, 290, {
        "chest.rot": (.10, 0, 0),
        "upper_arm.L.rot": (.10, 0, -.30),
        "upper_arm.R.rot": (.10, 0, .30),
        "forearm.L.rot": (-.10, 0, -.12),
        "forearm.R.rot": (-.10, 0, .12),
    })
    key_pose(rig, 302)

    # Skill Two -- gravity snare: orbit the staff and free hand around the core.
    key_pose(rig, 304)
    key_pose(rig, 324, {
        "hips.loc": (0, .008 * height, -.015 * height),
        "chest.rot": (-.08, -.24, 0),
        "head.rot": (.04, .10, 0),
        "upper_arm.L.rot": (-.38, .04, -.92),
        "forearm.L.rot": (-.62, 0, -.58),
        "upper_arm.R.rot": (-.82, -.04, -.52),
        "forearm.R.rot": (-.88, 0, -.24),
        "thigh.L.rot": (.14, 0, 0),
        "thigh.R.rot": (.14, 0, 0),
        "shin.L.rot": (-.20, 0, 0),
        "shin.R.rot": (-.20, 0, 0),
    })
    snare_contact = {
        "hips.loc": (0, -.025 * height, -.018 * height),
        "chest.rot": (.10, .24, 0),
        "head.rot": (-.05, -.10, 0),
        "upper_arm.L.rot": (-.50, -.02, -.88),
        "forearm.L.rot": (-.20, 0, -.34),
        "upper_arm.R.rot": (-.34, .02, .54),
        "forearm.R.rot": (-.52, 0, .26),
        "thigh.L.rot": (.16, 0, 0),
        "thigh.R.rot": (.16, 0, 0),
        "shin.L.rot": (-.22, 0, 0),
        "shin.R.rot": (-.22, 0, 0),
    }
    key_pose(rig, 344, snare_contact)
    key_pose(rig, 364, {
        "hips.rot": (0, 0, .12),
        "chest.rot": (.05, 0, .24),
        "upper_arm.L.rot": (.12, 0, .28),
        "upper_arm.R.rot": (.16, 0, .22),
    })
    key_pose(rig, 382)

    # Ultimate -- channel overhead, then detonate with a strong chest recoil.
    key_pose(rig, 384)
    key_pose(rig, 408, {
        "hips.loc": (0, .015 * height, -.040 * height),
        "chest.rot": (-.32, 0, 0),
        "head.rot": (.18, 0, 0),
        "upper_arm.L.rot": (-1.18, .08, -.56),
        "forearm.L.rot": (-1.06, 0, -.30),
        "upper_arm.R.rot": (-1.18, -.08, .56),
        "forearm.R.rot": (-1.06, 0, .30),
        "thigh.L.rot": (.24, 0, 0),
        "thigh.R.rot": (.24, 0, 0),
        "shin.L.rot": (-.34, 0, 0),
        "shin.R.rot": (-.34, 0, 0),
    })
    key_pose(rig, 432, {
        "hips.loc": (0, 0, .012 * height),
        "chest.rot": (-.42, 0, 0),
        "head.rot": (.24, 0, 0),
        "upper_arm.L.rot": (-1.34, 0, -.24),
        "forearm.L.rot": (-1.08, 0, -.14),
        "upper_arm.R.rot": (-1.34, 0, .24),
        "forearm.R.rot": (-1.08, 0, .14),
    })
    ultimate_contact = {
        "hips.loc": (0, -.050 * height, -.020 * height),
        "chest.rot": (.16, .14, 0),
        "head.rot": (-.10, -.06, 0),
        "upper_arm.L.rot": (-.58, 0, -1.04),
        "forearm.L.rot": (-.16, 0, -.32),
        "upper_arm.R.rot": (-.72, 0, .48),
        "forearm.R.rot": (-.82, 0, .24),
        "thigh.L.rot": (.20, 0, 0),
        "thigh.R.rot": (.20, 0, 0),
        "shin.L.rot": (-.26, 0, 0),
        "shin.R.rot": (-.26, 0, 0),
    }
    key_pose(rig, 448, ultimate_contact)
    key_pose(rig, 466, {
        "chest.rot": (.14, 0, 0),
        "head.rot": (-.06, 0, 0),
        "upper_arm.L.rot": (.18, 0, -.34),
        "upper_arm.R.rot": (.18, 0, .34),
    })
    key_pose(rig, 478)

    # Dash special -- a compact blink pose followed by a forward casting thrust.
    key_pose(rig, 480)
    key_pose(rig, 492, {
        "hips.loc": (0, .020 * height, -.025 * height),
        "chest.rot": (-.20, 0, -.20),
        "head.rot": (.10, 0, .08),
        "upper_arm.L.rot": (-.66, 0, -.46),
        "forearm.L.rot": (-.76, 0, -.26),
        "upper_arm.R.rot": (-.72, 0, .40),
        "forearm.R.rot": (-.82, 0, .22),
        "thigh.L.rot": (-.24, 0, 0),
        "thigh.R.rot": (.24, 0, 0),
    })
    dash_contact = {
        "hips.loc": (0, -.065 * height, 0),
        "chest.rot": (.38, 0, .18),
        "head.rot": (-.20, 0, -.08),
        "upper_arm.L.rot": (-.46, 0, -.74),
        "forearm.L.rot": (.06, 0, -.34),
        "upper_arm.R.rot": (-.42, 0, .70),
        "forearm.R.rot": (-.08, 0, .32),
        "thigh.L.rot": (.38, 0, 0),
        "thigh.R.rot": (-.38, 0, 0),
        "shin.L.rot": (-.20, 0, 0),
        "shin.R.rot": (-.10, 0, 0),
    }
    key_pose(rig, 505, dash_contact)
    key_pose(rig, 518, {
        "hips.loc": (0, -.015 * height, 0),
        "chest.rot": (.10, 0, .04),
        "upper_arm.L.rot": (.12, 0, -.16),
        "upper_arm.R.rot": (.10, 0, .16),
        "thigh.L.rot": (.10, 0, 0),
        "thigh.R.rot": (-.10, 0, 0),
    })
    key_pose(rig, 526)


def matrix_from_bone_segment(head, tail, reference=Vector((0, 0, 1))):
    """Build an armature-space bone matrix whose local +Y follows a segment."""
    head, tail = Vector(head), Vector(tail)
    axis_y = (tail - head).normalized()
    axis_x = Vector(reference).cross(axis_y)
    if axis_x.length_squared < 1e-8:
        axis_x = Vector((0, 1, 0)).cross(axis_y)
    axis_x.normalize()
    axis_z = axis_x.cross(axis_y).normalized()
    return Matrix((
        (axis_x.x, axis_y.x, axis_z.x, head.x),
        (axis_x.y, axis_y.y, axis_z.y, head.y),
        (axis_x.z, axis_y.z, axis_z.z, head.z),
        (0, 0, 0, 1),
    ))


def key_current_pose_bone(bone, frame):
    bone.rotation_mode = "XYZ"
    bone.keyframe_insert("location", frame=frame, group=bone.name)
    bone.keyframe_insert("rotation_euler", frame=frame, group=bone.name)
    bone.keyframe_insert("scale", frame=frame, group=bone.name)


def place_two_bone_arm(rig, side, wrist_target, elbow_pole, frame):
    """Place an arm by wrist/pole targets and bake the result to Euler keys.

    Targets use armature space with -Y as gameplay/camera forward.  This avoids
    mirrored Euler guesses and guarantees that a spell hand described as
    "forward" actually sits in front of the torso in the exported animation.
    """
    upper = rig.pose.bones[f"upper_arm.{side}"]
    forearm = rig.pose.bones[f"forearm.{side}"]
    hand = rig.pose.bones[f"hand.{side}"]
    shoulder = upper.head.copy()
    wrist_target = Vector(wrist_target)
    elbow_pole = Vector(elbow_pole)
    upper_length = upper.bone.length
    forearm_length = forearm.bone.length
    shoulder_to_target = wrist_target - shoulder
    distance = shoulder_to_target.length
    minimum = abs(upper_length - forearm_length) + 1e-4
    maximum = upper_length + forearm_length - 1e-4
    distance = max(minimum, min(maximum, distance))
    direction = shoulder_to_target.normalized()
    wrist = shoulder + direction * distance
    along = (
        upper_length * upper_length
        - forearm_length * forearm_length
        + distance * distance
    ) / (2.0 * distance)
    height_sq = max(0.0, upper_length * upper_length - along * along)
    bend = elbow_pole - shoulder
    bend -= direction * bend.dot(direction)
    if bend.length_squared < 1e-8:
        bend = Vector((1 if side == "R" else -1, 0, 0))
        bend -= direction * bend.dot(direction)
    bend.normalize()
    elbow = shoulder + direction * along + bend * math.sqrt(height_sq)

    upper.matrix = matrix_from_bone_segment(shoulder, elbow)
    bpy.context.view_layer.update()
    forearm.matrix = matrix_from_bone_segment(elbow, wrist)
    bpy.context.view_layer.update()
    hand_direction = (wrist - elbow).normalized()
    hand.matrix = matrix_from_bone_segment(
        wrist, wrist + hand_direction * hand.bone.length
    )
    bpy.context.view_layer.update()
    for bone in (upper, forearm, hand):
        key_current_pose_bone(bone, frame)
    return wrist


def orient_weapon_socket(rig, direction, frame):
    """Bake a grip-preserving socket rotation with weapon local +Z on direction."""
    socket = rig.pose.bones["weapon_socket.R"]
    grip = socket.matrix.translation.copy()
    axis_z = Vector(direction).normalized()
    axis_x = Vector((0, 0, 1)).cross(axis_z)
    if axis_x.length_squared < 1e-8:
        axis_x = Vector((1, 0, 0))
    axis_x.normalize()
    axis_y = axis_z.cross(axis_x).normalized()
    socket.matrix = Matrix((
        (axis_x.x, axis_y.x, axis_z.x, grip.x),
        (axis_x.y, axis_y.y, axis_z.y, grip.y),
        (axis_x.z, axis_y.z, axis_z.z, grip.z),
        (0, 0, 0, 1),
    ))
    bpy.context.view_layer.update()
    key_current_pose_bone(socket, frame)


def targeted_combat_contacts(actor, rig, height):
    """Bake target-driven contact poses after authored Euler/body animation."""
    if actor["style"] == "knight":
        contacts = [
            ([188, 189], (-.18, -.22, .55), (.20, -.21, .52),
             (1.0, -.30, .10)),
            ([274, 275], (-.18, -.20, .58), (.18, -.18, .58),
             (.05, -.20, .98)),
            ([344, 345], (-.18, -.24, .60), (.19, -.20, .62),
             (0, -.40, -.90)),
            ([448, 449], (-.17, -.22, .60), (.17, -.24, .62),
             (0, -.45, -.89)),
            ([505, 506], (-.18, -.21, .53), (.19, -.25, .54),
             (0, -.98, .20)),
        ]
    else:
        contacts = [
            ([188, 189], (-.20, -.24, .61), (.18, -.20, .60),
             (0, -.94, .34)),
            ([274, 275], (-.21, -.26, .64), (.18, -.20, .64),
             (.04, -.40, .92)),
            ([344, 345], (-.20, -.24, .60), (.20, -.22, .62),
             (.64, -.52, .56)),
            ([448, 449], (-.22, -.27, .65), (.18, -.22, .66),
             (.05, -.64, .77)),
            ([505, 506], (-.20, -.25, .59), (.20, -.24, .61),
             (0, -.98, .15)),
        ]

    left_pole = Vector((-.46 * height, -.08 * height, .58 * height))
    right_pole = Vector((.46 * height, -.08 * height, .58 * height))
    scene = bpy.context.scene
    for frames, left_target, right_target, weapon_direction in contacts:
        for frame in frames:
            scene.frame_set(frame)
            bpy.context.view_layer.update()
            place_two_bone_arm(
                rig,
                "L",
                Vector(left_target) * height,
                left_pole,
                frame,
            )
            place_two_bone_arm(
                rig,
                "R",
                Vector(right_target) * height,
                right_pole,
                frame,
            )
            direction = (
                -Vector(weapon_direction)
                if actor["style"] == "knight"
                else Vector(weapon_direction)
            )
            orient_weapon_socket(rig, direction, frame)


def audit_combat_contacts(actor, rig, height):
    """Regression guard for target-authored hands and modular weapon motion."""
    scene = bpy.context.scene
    action = rig.animation_data.action
    socket_path = 'pose.bones["weapon_socket.R"].rotation_euler'
    socket_curves = [
        curve for curve in action.fcurves if curve.data_path == socket_path
    ]
    socket_keyframes = sorted({
        int(round(point.co.x))
        for curve in socket_curves
        for point in curve.keyframe_points
    })
    required_hold_frames = {
        frame for _, contact in COMBAT_CONTACTS
        for frame in (contact, contact + 1)
    }
    missing_hold_frames = sorted(required_hold_frames - set(socket_keyframes))
    if missing_hold_frames:
        raise RuntimeError(
            f"{actor['id']} socket is missing contact-hold keys: "
            f"{missing_hold_frames}"
        )

    # key_pose() must not write the socket.  An all-zero rotation inside a
    # combat clip is the signature of the old body-pose identity reset/pop.
    identity_reset_frames = []
    for frame in socket_keyframes:
        if not any(first < frame < last for _, first, last in CLIPS[2:7]):
            continue
        values = [curve.evaluate(frame) for curve in socket_curves]
        if len(values) == 3 and max(abs(value) for value in values) < 1e-5:
            identity_reset_frames.append(frame)
    if identity_reset_frames:
        raise RuntimeError(
            f"{actor['id']} socket contains identity reset keys inside combat: "
            f"{identity_reset_frames}"
        )

    samples = []
    minimum_hand_forward = math.inf
    maximum_forward_length_error = 0.0
    for clip, frame in COMBAT_CONTACTS:
        scene.frame_set(frame)
        bpy.context.view_layer.update()
        chest = rig.pose.bones["chest"].matrix.translation.copy()
        left = rig.pose.bones["hand.L"].matrix.translation.copy()
        right = rig.pose.bones["hand.R"].matrix.translation.copy()
        socket = rig.pose.bones["weapon_socket.R"]
        raw_forward = socket.matrix.to_3x3() @ Vector((0, 0, 1))
        raw_length = raw_forward.length
        finite = all(math.isfinite(value) for value in raw_forward)
        if not finite or raw_length < 1e-8:
            raise RuntimeError(
                f"{actor['id']} has an invalid socket vector at frame {frame}"
            )
        socket_forward = raw_forward / raw_length
        length_error = abs(socket_forward.length - 1.0)
        maximum_forward_length_error = max(
            maximum_forward_length_error, length_error
        )

        # Gameplay/camera forward is armature -Y.  Both target-authored hands
        # must remain visibly in front of the chest at the release/contact.
        left_forward = chest.y - left.y
        right_forward = chest.y - right.y
        minimum_hand_forward = min(
            minimum_hand_forward, left_forward, right_forward
        )
        required_forward = .015 * height
        if left_forward < required_forward or right_forward < required_forward:
            raise RuntimeError(
                f"{actor['id']} contact {clip}@{frame} retracts a hand behind "
                f"the torso: L={left_forward:.5f}, R={right_forward:.5f}"
            )
        samples.append({
            "clip": clip,
            "frame": frame,
            "handL": [round(value, 5) for value in left],
            "handR": [round(value, 5) for value in right],
            "handForwardFromChest": [
                round(left_forward, 5), round(right_forward, 5)
            ],
            "socketForward": [round(value, 6) for value in socket_forward],
        })

    return {
        "status": "pass",
        "coordinateSpace": "armature_-Y_forward",
        "contacts": samples,
        "minimumHandForwardMeters": round(minimum_hand_forward, 5),
        "socketForwardMaximumLengthError": round(
            maximum_forward_length_error, 8
        ),
        "socketRotationKeyframes": socket_keyframes,
        "identityResetFramesInsideCombat": identity_reset_frames,
        "contactHoldFrames": sorted(required_hold_frames),
    }


def create_animation(actor, mesh, rig):
    for action in list(bpy.data.actions):
        bpy.data.actions.remove(action)
    bounds = mesh_bounds(mesh)
    height = bounds[5] - bounds[4]
    action = bpy.data.actions.new(actor["id"] + "_Master_60FPS")
    rig.animation_data_create()
    rig.animation_data.action = action
    humanoid_animation(actor, rig, height)
    if actor["style"] == "knight":
        knight_combat_animation(rig, height)
        knight_weapon_animation(rig)
    elif actor["style"] == "caster":
        magic_caster_combat_animation(rig, height)
        magic_caster_weapon_animation(rig)
        magic_caster_run_staff_animation(rig, height)
    targeted_combat_contacts(actor, rig, height)
    for curve in action.fcurves:
        for key in curve.keyframe_points:
            key.interpolation = "BEZIER"
            key.handle_left_type = "AUTO_CLAMPED"
            key.handle_right_type = "AUTO_CLAMPED"
    configure_scene()
    return action


def bake_grounding(actor, mesh, rig, action):
    scene = bpy.context.scene
    root = rig.pose.bones["root"]
    samples = []
    minimum_before = 0.0
    maximum_lift = 0.0
    for frame in range(655):
        scene.frame_set(frame)
        evaluated = mesh.evaluated_get(bpy.context.evaluated_depsgraph_get())
        evaluated_mesh = evaluated.to_mesh()
        minimum_z = min(
            (evaluated.matrix_world @ vertex.co).z
            for vertex in evaluated_mesh.vertices
        )
        evaluated.to_mesh_clear()
        location = root.location.copy()
        lift = .002 - minimum_z
        minimum_before = min(minimum_before, minimum_z)
        maximum_lift = max(maximum_lift, lift)
        samples.append((frame, location, lift))

    for frame, location, lift in samples:
        scene.frame_set(frame)
        world_delta = Vector((0, 0, lift))
        armature_delta = rig.matrix_world.to_3x3().inverted() @ world_delta
        local_delta = root.bone.matrix_local.to_3x3().inverted() @ armature_delta
        root.location = location + local_delta
        root.keyframe_insert("location", frame=frame, group="root")
    root_path = 'pose.bones["root"].location'
    for curve in action.fcurves:
        if curve.data_path == root_path:
            for key in curve.keyframe_points:
                key.interpolation = "LINEAR"
    scene.frame_set(0)
    print(
        f"grounded {actor['id']} minimumBefore={minimum_before:.5f} maximumLift={maximum_lift:.5f}",
        flush=True,
    )
    return {
        "minimumBefore": round(minimum_before, 5),
        "maximumLift": round(maximum_lift, 5),
    }


def audit_deformation(actor, mesh):
    """Fail on ground penetration and the needle-like stretching seen in bad weights."""
    scene = bpy.context.scene
    rest_points = [mesh.matrix_world @ vertex.co for vertex in mesh.data.vertices]
    rest_min_z = min(point.z for point in rest_points)
    rest_max_z = max(point.z for point in rest_points)
    height = rest_max_z - rest_min_z
    edges = []
    for edge in mesh.data.edges:
        left, right = edge.vertices
        length = (rest_points[left] - rest_points[right]).length
        # Ignore numerical/coincident edges, while keeping the short edges that
        # turn into the visible needles produced by a bad weight boundary.
        if height * .001 <= length <= height * .15:
            edges.append((left, right, length))

    frames = set(range(0, 655, 4))
    frames.update(frame for frame, _ in QA_POSES)
    for _, first, last in CLIPS:
        frames.update((first, last))
    frames.add(654)

    maximum_ratio = 1.0
    maximum_frame = 0
    maximum_edge = (-1, -1)
    maximum_deformed_length = 0.0
    minimum_z = float("inf")
    maximum_below_ground = 0
    for frame in sorted(frames):
        scene.frame_set(frame)
        evaluated = mesh.evaluated_get(bpy.context.evaluated_depsgraph_get())
        evaluated_mesh = evaluated.to_mesh()
        points = [evaluated.matrix_world @ vertex.co for vertex in evaluated_mesh.vertices]
        evaluated.to_mesh_clear()
        frame_min_z = min(point.z for point in points)
        below_ground = sum(1 for point in points if point.z < -.002)
        minimum_z = min(minimum_z, frame_min_z)
        maximum_below_ground = max(maximum_below_ground, below_ground)
        for left, right, rest_length in edges:
            deformed_length = (points[left] - points[right]).length
            ratio = deformed_length / rest_length
            if ratio > maximum_ratio:
                maximum_ratio = ratio
                maximum_frame = frame
                maximum_edge = (left, right)
                maximum_deformed_length = deformed_length

    # The Caster's broad cape legitimately stretches farther than armor/hair,
    # but the old hard rear-weight boundary measured 128-134x.  These limits
    # leave ample room for cloth-like motion while rejecting that failure.
    ratio_limit = 16.0 if actor["style"] == "knight" else 40.0
    if maximum_ratio > ratio_limit:
        raise RuntimeError(
            f"{actor['id']} deformation spike: edge {maximum_edge} stretches "
            f"{maximum_ratio:.3f}x at frame {maximum_frame} "
            f"({maximum_deformed_length:.4f}m); limit={ratio_limit:.1f}x"
        )
    if minimum_z < -.002 or maximum_below_ground:
        raise RuntimeError(
            f"{actor['id']} penetrates the floor after grounding: "
            f"minimumZ={minimum_z:.6f}, belowGroundVertices={maximum_below_ground}"
        )
    scene.frame_set(0)
    return {
        "sampledFrameCount": len(frames),
        "minimumZ": round(minimum_z, 6),
        "maximumBelowGroundVertices": maximum_below_ground,
        "maximumEdgeStretch": round(maximum_ratio, 5),
        "maximumEdgeStretchFrame": maximum_frame,
        "maximumEdgeVertices": list(maximum_edge),
        "maximumDeformedEdgeMeters": round(maximum_deformed_length, 5),
        "edgeStretchLimit": ratio_limit,
    }


def look_at(obj, target):
    obj.rotation_euler = (Vector(target) - obj.location).to_track_quat("-Z", "Y").to_euler()


def load_qa_weapon(actor):
    path = resolve_weapon_input(actor)
    if path is None:
        return None, {
            "status": "pending_input",
            "expectedInputs": [relative(item) for item in weapon_input_candidates(actor)],
        }

    before = set(bpy.data.objects)
    extension = os.path.splitext(path)[1].lower()
    if extension == ".blend":
        with bpy.data.libraries.load(path, link=False) as (source, target):
            mesh_names = []
            for name in source.objects:
                source_object = source.objects and name
                if source_object:
                    mesh_names.append(name)
            target.objects = mesh_names
        for obj in target.objects:
            if obj is not None:
                bpy.context.scene.collection.objects.link(obj)
    elif extension in {".glb", ".gltf"}:
        bpy.ops.import_scene.gltf(filepath=path)
    else:
        raise RuntimeError(f"Unsupported QA weapon input: {path}")

    imported = [obj for obj in bpy.data.objects if obj not in before]
    meshes = [obj for obj in imported if obj.type == "MESH"]
    if len(meshes) != 1:
        raise RuntimeError(
            f"QA weapon {path} must contain exactly one mesh; found "
            f"{[(obj.name, obj.type) for obj in imported]}"
        )
    if any(obj.type == "ARMATURE" for obj in imported):
        raise RuntimeError(f"Modular weapon must not contain an armature: {path}")
    weapon = meshes[0]
    if (
        weapon.location.length > 1e-5
        or max(abs(value) for value in weapon.rotation_euler) > 1e-5
        or max(abs(value - 1.0) for value in weapon.scale) > 1e-5
    ):
        raise RuntimeError(
            f"QA weapon transform must be identity before socket attachment: {path}"
        )
    local_points = [vertex.co.copy() for vertex in weapon.data.vertices]
    local_bounds = (
        min(point.x for point in local_points), max(point.x for point in local_points),
        min(point.y for point in local_points), max(point.y for point in local_points),
        min(point.z for point in local_points), max(point.z for point in local_points),
    )
    for minimum, maximum, axis in zip(local_bounds[::2], local_bounds[1::2], "XYZ"):
        if minimum > .01 or maximum < -.01:
            raise RuntimeError(
                f"{actor['id']} weapon grip origin lies outside its {axis} bounds: "
                f"[{minimum:.5f}, {maximum:.5f}]"
            )
    weapon.name = "TEMP_QA_EquippedWeapon"
    return weapon, {
        "status": "complete",
        "input": relative(path),
        "object": weapon.name,
        "vertices": len(weapon.data.vertices),
        "boundsMeters": [
            round(local_bounds[1] - local_bounds[0], 5),
            round(local_bounds[3] - local_bounds[2], 5),
            round(local_bounds[5] - local_bounds[4], 5),
        ],
        "pivot": "grip_at_local_origin",
    }


def remove_qa_weapon(weapon):
    if weapon is None:
        return
    mesh_data = weapon.data
    bpy.data.objects.remove(weapon, do_unlink=True)
    if mesh_data.users == 0:
        bpy.data.meshes.remove(mesh_data)


def render_qa(
    actor,
    mesh,
    rig,
    frame,
    label,
    three_quarter=False,
    rear_view=False,
    weapon=None,
):
    os.makedirs(QA_ROOT, exist_ok=True)
    scene = bpy.context.scene
    scene.frame_set(frame)
    if weapon is not None:
        weapon.hide_render = False
        weapon.matrix_world = (
            rig.matrix_world @ rig.pose.bones["weapon_socket.R"].matrix
        )
        bpy.context.view_layer.update()
    evaluated = mesh.evaluated_get(bpy.context.evaluated_depsgraph_get())
    evaluated_mesh = evaluated.to_mesh()
    points = [evaluated.matrix_world @ vertex.co for vertex in evaluated_mesh.vertices]
    evaluated.to_mesh_clear()
    framing_points = list(points)
    weapon_points = []
    if weapon is not None:
        weapon_points = [
            weapon.matrix_world @ vertex.co for vertex in weapon.data.vertices
        ]
        framing_points.extend(weapon_points)
    xmin, xmax = min(point.x for point in framing_points), max(point.x for point in framing_points)
    ymin, ymax = min(point.y for point in framing_points), max(point.y for point in framing_points)
    zmin, zmax = min(point.z for point in framing_points), max(point.z for point in framing_points)
    width, depth, height = xmax - xmin, ymax - ymin, zmax - zmin
    target = Vector(((xmin + xmax) * .5, (ymin + ymax) * .5, (zmin + zmax) * .5))
    below_ground = sum(1 for point in points if point.z < -.01)
    weapon_below_ground = sum(1 for point in weapon_points if point.z < -.01)
    print(
        f"QA {actor['id']} {label} frame={frame} minZ={zmin:.5f} "
        f"belowGround={below_ground} weaponBelowGround={weapon_below_ground}",
        flush=True,
    )

    scene.render.resolution_x = 640
    scene.render.resolution_y = 800
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.film_transparent = False
    if scene.world is None:
        scene.world = bpy.data.worlds.new("ModularHeroQAWorld")
    scene.world.use_nodes = True
    background = scene.world.node_tree.nodes.get("Background")
    background.inputs["Color"].default_value = (.005, .007, .012, 1.0)
    background.inputs["Strength"].default_value = .18

    bpy.ops.mesh.primitive_plane_add(size=max(8.0, width * 4, depth * 4), location=(0, 0, -.012))
    floor = bpy.context.object
    floor.name = "TEMP_QA_Floor"
    floor_material = bpy.data.materials.new("TEMP_QA_FloorMaterial")
    floor_material.diffuse_color = (.020, .026, .042, 1.0)
    floor.data.materials.append(floor_material)

    lights = []
    for location, energy, color, size in [
        ((-3.0, -4.0, target.z + height * .45), 650, (1.0, .91, .82), 4.0),
        ((3.0, 0.0, target.z + height * .30), 520, (.75, .86, 1.0), 3.5),
        ((0.0, 2.0, target.z + height * .70), 320, (.90, .84, 1.0), 3.0),
    ]:
        data = bpy.data.lights.new("TEMP_QA_Light", "AREA")
        data.energy, data.color, data.shape, data.size = energy, color, "DISK", size
        light = bpy.data.objects.new("TEMP_QA_Light", data)
        scene.collection.objects.link(light)
        light.location = location
        look_at(light, target)
        lights.append(light)

    camera_data = bpy.data.cameras.new("TEMP_QA_Camera")
    camera = bpy.data.objects.new("TEMP_QA_Camera", camera_data)
    scene.collection.objects.link(camera)
    camera_distance = max(6.0, height * 2.8)
    camera.location = (
        target.x + camera_distance * .42 if three_quarter else target.x,
        target.y + camera_distance if rear_view else target.y - camera_distance,
        target.z,
    )
    look_at(camera, target)
    camera_data.type = "ORTHO"
    aspect = scene.render.resolution_x / scene.render.resolution_y
    camera_data.ortho_scale = max(height * 1.30, width / max(.1, aspect) * 1.22)
    scene.camera = camera
    filepath = os.path.join(QA_ROOT, f"{actor['id']}_{label}.png")
    scene.render.filepath = filepath
    scene.view_settings.look = "AgX - Medium High Contrast"
    scene.view_settings.exposure = -.12
    bpy.ops.render.render(write_still=True)
    if weapon is not None:
        weapon.hide_render = True

    floor_mesh = floor.data
    bpy.data.objects.remove(floor, do_unlink=True)
    bpy.data.meshes.remove(floor_mesh, do_unlink=True)
    bpy.data.materials.remove(floor_material, do_unlink=True)
    for light in lights:
        data = light.data
        bpy.data.objects.remove(light, do_unlink=True)
        bpy.data.lights.remove(data, do_unlink=True)
    bpy.data.objects.remove(camera, do_unlink=True)
    bpy.data.cameras.remove(camera_data, do_unlink=True)
    result = {
        "frame": frame,
        "path": relative(filepath),
        "minimumZ": round(zmin, 5),
        "belowGroundVertices": below_ground,
        "view": "rear" if rear_view else ("threeQuarter" if three_quarter else "front"),
        "equippedWeapon": weapon is not None,
    }
    if weapon is not None:
        result["weaponMinimumZ"] = round(
            min(point.z for point in weapon_points), 5
        )
        result["weaponBelowGroundVertices"] = weapon_below_ground
    return result


def pack_images():
    for image in bpy.data.images:
        if image.source != "FILE":
            continue
        try:
            image.pack()
        except RuntimeError as error:
            print(f"could not pack image {image.name!r}: {error}", flush=True)


def export_actor(actor, mesh, rig):
    blend_path, glb_path = output_paths(actor)
    os.makedirs(os.path.dirname(blend_path), exist_ok=True)
    pack_images()
    scene = bpy.context.scene
    scene.frame_start = 0
    scene.frame_end = 654
    scene.render.fps = 60
    scene.frame_set(0)
    bpy.context.preferences.filepaths.save_version = 0
    select_only([mesh, rig], active=rig)
    bpy.ops.wm.save_as_mainfile(filepath=blend_path, check_existing=False)
    select_only([mesh, rig], active=rig)
    bpy.ops.export_scene.gltf(
        filepath=glb_path,
        export_format="GLB",
        use_selection=True,
        export_yup=True,
        export_animations=True,
        export_animation_mode="ACTIONS",
        export_force_sampling=True,
        export_frame_range=True,
        export_frame_step=1,
        export_skins=True,
        export_def_bones=False,
        export_leaf_bone=False,
        export_all_influences=False,
        # glTF and raylib 6.0 both consume one four-wide JOINTS_0/WEIGHTS_0 set.
        export_influence_nb=4,
        export_morph=False,
        export_materials="EXPORT",
        export_cameras=False,
        export_lights=False,
        export_extras=True,
    )
    return blend_path, glb_path


def read_glb_json(path):
    with open(path, "rb") as handle:
        header = handle.read(12)
        if len(header) != 12:
            raise RuntimeError(f"Truncated GLB: {path}")
        magic, version, total_length = struct.unpack("<4sII", header)
        if magic != b"glTF" or version != 2:
            raise RuntimeError(f"Not a glTF 2.0 GLB: {path}")
        chunk_header = handle.read(8)
        chunk_length, chunk_type = struct.unpack("<I4s", chunk_header)
        if chunk_type != b"JSON":
            raise RuntimeError(f"First GLB chunk is not JSON: {path}")
        payload = handle.read(chunk_length).decode("utf-8").rstrip(" \t\r\n\0")
    if total_length != os.path.getsize(path):
        raise RuntimeError(f"GLB declared length mismatch: {path}")
    return json.loads(payload)


def gltf_node_local_rotation(node):
    if "matrix" in node:
        values = node["matrix"]
        matrix = Matrix((
            (values[0], values[4], values[8], values[12]),
            (values[1], values[5], values[9], values[13]),
            (values[2], values[6], values[10], values[14]),
            (values[3], values[7], values[11], values[15]),
        ))
        return matrix.to_quaternion().normalized()
    x, y, z, w = node.get("rotation", (0.0, 0.0, 0.0, 1.0))
    return Quaternion((w, x, y, z)).normalized()


def gltf_node_global_rotation(nodes, node_index):
    parent_by_child = {}
    for parent_index, node in enumerate(nodes):
        for child_index in node.get("children", []):
            parent_by_child[child_index] = parent_index

    chain = []
    current = node_index
    visited = set()
    while current is not None:
        if current in visited:
            raise RuntimeError("Cycle found in exported GLB node hierarchy")
        visited.add(current)
        chain.append(current)
        current = parent_by_child.get(current)

    rotation = Quaternion((1.0, 0.0, 0.0, 0.0))
    for index in reversed(chain):
        rotation = rotation @ gltf_node_local_rotation(nodes[index])
    return rotation.normalized()


def validate_exported_glb(path):
    document = read_glb_json(path)
    skins = document.get("skins", [])
    if len(skins) != 1:
        raise RuntimeError(f"Expected one GLB skin; found {len(skins)}")
    nodes = document.get("nodes", [])
    joint_indices = skins[0].get("joints", [])
    joint_names = [nodes[index].get("name", "") for index in joint_indices]
    if len(joint_indices) != 16 or set(joint_names) != set(EXPECTED_JOINTS):
        raise RuntimeError(
            f"Exported skin.joints violates 16-joint contract: {joint_names}"
        )
    hand_index = next(
        index for index in joint_indices if nodes[index].get("name") == "hand.R"
    )
    socket_index = next(
        index for index in joint_indices
        if nodes[index].get("name") == "weapon_socket.R"
    )
    if socket_index not in nodes[hand_index].get("children", []):
        raise RuntimeError("Exported weapon_socket.R is not a direct hand.R child")
    socket_bind_rotation = gltf_node_global_rotation(nodes, socket_index)
    socket_bind_angle = math.degrees(
        2.0 * math.acos(max(-1.0, min(1.0, abs(socket_bind_rotation.w))))
    )
    if socket_bind_angle > .1:
        raise RuntimeError(
            "Exported weapon_socket.R bind rotation must be identity in model space; "
            f"got {socket_bind_angle:.6f} degrees"
        )
    if not document.get("animations"):
        raise RuntimeError("Exported body GLB contains no animation")
    mesh_node_names = [node.get("name", "") for node in nodes if "mesh" in node]
    if any(any(word in name.lower() for word in WEAPON_WORDS) for name in mesh_node_names):
        raise RuntimeError(f"Weapon-like mesh leaked into body GLB: {mesh_node_names}")
    return {
        "skinJointCount": len(joint_indices),
        "skinJoints": joint_names,
        "socketBindRotationDegrees": round(socket_bind_angle, 6),
        "animationCount": len(document.get("animations", [])),
        "meshNodes": mesh_node_names,
    }


def process_actor(actor, input_path, no_render=False):
    print(f"\n=== MODULAR RIG {actor['id']} ===", flush=True)
    print(f"input={input_path}", flush=True)
    mesh = load_body(actor, input_path)
    bounds = normalize_body(actor, mesh)
    validate_builder_surface(actor, mesh)
    body_geometry_report = audit_body_only_geometry(actor, mesh, bounds)
    mesh.name = actor["object"]
    mesh.data.name = actor["object"] + "Mesh"
    mesh["asset_role"] = "body_only"
    mesh["weapons_embedded"] = False
    mesh["source_builder"] = relative(input_path)

    rig, specs = create_rig(actor, mesh)
    skin_report = skin_body(actor, mesh, rig, specs)
    validate_rig(actor, rig)
    weight_report = audit_weights(mesh)
    if weight_report["unweightedVertices"]:
        raise RuntimeError(
            f"{actor['id']} has {weight_report['unweightedVertices']} unweighted vertices"
        )
    if weight_report["forbiddenSocketWeightedVertices"]:
        raise RuntimeError("Body vertices must never be weighted to weapon_socket.R")
    if weight_report["maximumInfluences"] > 4:
        raise RuntimeError(
            f"{actor['id']} exceeds raylib's four-influence limit: "
            f"{weight_report['maximumInfluences']}"
        )
    if weight_report["minimumWeightSum"] < .999:
        raise RuntimeError(
            f"{actor['id']} has non-normalized weights: {weight_report['minimumWeightSum']}"
        )

    modifier = next(item for item in mesh.modifiers if item.type == "ARMATURE")
    modifier.show_viewport = False
    action = create_animation(actor, mesh, rig)
    modifier.show_viewport = True
    grounding = bake_grounding(actor, mesh, rig, action)
    deformation = audit_deformation(actor, mesh)
    combat_contact_audit = audit_combat_contacts(
        actor, rig, bounds[5] - bounds[4]
    )

    qa_results = []
    qa_weapon, weapon_qa_report = load_qa_weapon(actor)
    if qa_weapon is not None:
        qa_weapon.hide_render = True
    if not no_render:
        for frame, label in QA_POSES:
            qa_results.append(render_qa(actor, mesh, rig, frame, label))
        qa_results.append(
            render_qa(
                actor, mesh, rig, 190, "basic_three_quarter", three_quarter=True
            )
        )
        qa_results.append(
            render_qa(
                actor,
                mesh,
                rig,
                344 if actor["style"] == "knight" else 448,
                "rear_deformation",
                three_quarter=True,
                rear_view=True,
            )
        )
        if qa_weapon is not None:
            qa_results.append(
                render_qa(
                    actor,
                    mesh,
                    rig,
                    0,
                    "equipped_rest",
                    three_quarter=True,
                    weapon=qa_weapon,
                )
            )
            qa_results.append(
                render_qa(
                    actor,
                    mesh,
                    rig,
                    120,
                    "equipped_run_opposite",
                    three_quarter=True,
                    weapon=qa_weapon,
                )
            )
            for frame, label in COMBAT_QA_POSES:
                qa_results.append(
                    render_qa(
                        actor,
                        mesh,
                        rig,
                        frame,
                        "equipped_" + label,
                        three_quarter=True,
                        weapon=qa_weapon,
                    )
                )
            qa_results.append(
                render_qa(
                    actor,
                    mesh,
                    rig,
                    104,
                    "equipped_run",
                    three_quarter=True,
                    weapon=qa_weapon,
                )
            )
            qa_results.append(
                render_qa(
                    actor,
                    mesh,
                    rig,
                    190,
                    "equipped_basic",
                    three_quarter=True,
                    weapon=qa_weapon,
                )
            )
            qa_results.append(
                render_qa(
                    actor,
                    mesh,
                    rig,
                    448,
                    "equipped_ultimate_rear",
                    three_quarter=True,
                    rear_view=True,
                    weapon=qa_weapon,
                )
            )
            bad_weapon_grounding = [
                item for item in qa_results
                if item.get("equippedWeapon")
                and item.get("weaponBelowGroundVertices", 0) > 0
            ]
            if bad_weapon_grounding:
                raise RuntimeError(
                    f"{actor['id']} equipped weapon crosses the ground in QA: "
                    f"{[(item['frame'], item['weaponMinimumZ']) for item in bad_weapon_grounding]}"
                )
    remove_qa_weapon(qa_weapon)

    blend_path, glb_path = export_actor(actor, mesh, rig)
    glb_report = validate_exported_glb(glb_path)
    triangles = sum(len(polygon.vertices) - 2 for polygon in mesh.data.polygons)
    result = {
        "id": actor["id"],
        "status": "complete",
        "input": relative(input_path),
        "bodyOnly": True,
        "object": mesh.name,
        "vertices": len(mesh.data.vertices),
        "triangles": triangles,
        "boundsMeters": [
            round(bounds[1] - bounds[0], 5),
            round(bounds[3] - bounds[2], 5),
            round(bounds[5] - bounds[4], 5),
        ],
        "bodyGeometry": body_geometry_report,
        "joints": len(rig.data.bones),
        "jointNames": [bone.name for bone in rig.data.bones],
        "socket": {"name": "weapon_socket.R", "parent": "hand.R"},
        "weaponPivotContract": {
            "pivot": "grip_at_local_origin",
            "runtimeGripOffset": "identity",
            "socketBindRotation": "identity_model_space",
            "weaponSourceAxes": "+Z_up_-Y_forward",
        },
        "frames": [int(action.frame_range[0]), int(action.frame_range[1])],
        "clips": [dict(id=name, start=first, end=last) for name, first, last in CLIPS],
        "weights": weight_report,
        "skinning": skin_report,
        "grounding": grounding,
        "deformation": deformation,
        "combatContactAudit": combat_contact_audit,
        "weaponQa": weapon_qa_report,
        "qa": qa_results,
        "blend": relative(blend_path),
        "glb": relative(glb_path),
        "glbAudit": glb_report,
    }
    print("MODULAR_HERO_RESULT=" + json.dumps(result), flush=True)
    return result


def write_report(processed, pending):
    merged = {}
    if os.path.isfile(REPORT_PATH):
        try:
            existing = json.load(open(REPORT_PATH, encoding="utf-8"))
            merged = {item["id"]: item for item in existing.get("actors", [])}
        except (OSError, ValueError, KeyError):
            merged = {}
    for result in processed:
        merged[result["id"]] = result
    for actor_id, candidates in pending.items():
        merged[actor_id] = {
            "id": actor_id,
            "status": "pending_input",
            "expectedInputs": [relative(path) for path in candidates],
        }
    report = {
        "schemaVersion": 1,
        "pipeline": "production_v3_modular_body_rig_v1",
        "status": "complete" if not pending else ("partial" if processed else "pending_input"),
        "sampleRate": 60,
        "totalFrames": 655,
        "jointContract": {
            "count": 16,
            "names": EXPECTED_JOINTS,
            "weaponSocket": "weapon_socket.R",
            "weaponSocketParent": "hand.R",
            "bodySocketWeights": 0,
            "weaponPivot": "grip_at_local_origin",
            "runtimeGripOffset": "identity",
            "socketBindRotation": "identity_model_space",
            "weaponSourceAxes": "+Z_up_-Y_forward",
        },
        "clips": [dict(id=name, start=first, end=last) for name, first, last in CLIPS],
        "actors": [merged[name] for name in ACTORS if name in merged],
    }
    os.makedirs(os.path.dirname(REPORT_PATH), exist_ok=True)
    with open(REPORT_PATH, "w", encoding="utf-8") as handle:
        json.dump(report, handle, indent=2)
        handle.write("\n")
    return report


def main():
    global OUTPUT_ROOT, QA_ROOT, REPORT_PATH
    args = parse_args()
    if args.output_root:
        OUTPUT_ROOT = os.path.abspath(os.path.join(ROOT, args.output_root))
        QA_ROOT = os.path.join(OUTPUT_ROOT, "qa", "rigged")
        REPORT_PATH = os.path.join(OUTPUT_ROOT, "modular_hero_rig_report.json")
    requested = list(ACTORS) if args.actor == "all" else [args.actor]
    processed = []
    pending = {}
    for actor_id in requested:
        actor = ACTORS[actor_id]
        input_path = resolve_input(actor, args.input)
        if input_path is None:
            candidates = (
                [os.path.abspath(os.path.join(ROOT, args.input))]
                if args.input else input_candidates(actor)
            )
            pending[actor_id] = candidates
            print(
                "MODULAR_HERO_PENDING_INPUT=" + json.dumps({
                    "id": actor_id,
                    "expected": [relative(path) for path in candidates],
                }),
                flush=True,
            )
            continue
        processed.append(process_actor(actor, input_path, no_render=args.no_render))

    report = write_report(processed, pending)
    print("MODULAR_HERO_REPORT=" + relative(REPORT_PATH), flush=True)
    if pending and args.strict_missing:
        raise RuntimeError(
            "Missing requested builder inputs: " + ", ".join(sorted(pending))
        )
    if report["status"] == "complete":
        print("MODULAR HERO RIG PIPELINE COMPLETE", flush=True)
    else:
        print("MODULAR HERO RIG PIPELINE WAITING FOR BUILDER INPUT", flush=True)


if __name__ == "__main__":
    main()
