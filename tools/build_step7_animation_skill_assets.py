"""Build and validate Step 7 named animation clips and 3D skill assets.

This is an offline Blender 4.5 LTS authoring pipeline.  It never runs inside
the C++ game.  Existing 655-frame runtime models are deliberately left intact;
the named-action exports live beside them until the C++ runtime opts into
clip-by-name playback.

Run from the repository root::

    blender --background --python tools/build_step7_animation_skill_assets.py

Useful development switches::

    -- --actor knight --skip-skills --no-render
    -- --skip-actors
    -- --validate-only
"""

from __future__ import annotations

import argparse
import bpy
import hashlib
import json
import math
import os
import struct
import sys
from mathutils import Vector


ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
ASSET_ROOT = os.path.join(ROOT, "assets", "survival3d")
STEP7_ROOT = os.path.join(ASSET_ROOT, "production_v3", "step7")
ACTOR_MODEL_ROOT = os.path.join(ASSET_ROOT, "models", "step7", "actors")
ACTOR_BLEND_ROOT = os.path.join(ASSET_ROOT, "source", "blender", "step7", "actors")
SKILL_MODEL_ROOT = os.path.join(ASSET_ROOT, "models", "skills")
SKILL_BLEND_ROOT = os.path.join(ASSET_ROOT, "source", "blender", "skills")
QA_ROOT = os.path.join(STEP7_ROOT, "qa", "skills")
ANIMATION_MANIFEST = os.path.join(ASSET_ROOT, "config", "animation_clips_step7.json")
SKILL_MANIFEST = os.path.join(ASSET_ROOT, "config", "skill_asset_manifest.json")
STEP7_MANIFEST = os.path.join(STEP7_ROOT, "step7_asset_manifest.json")
ACCEPTANCE_REPORT = os.path.join(ASSET_ROOT, "production_v3", "step7_acceptance_report.json")

PIPELINE_ID = "aegis_rift_step7_named_actions_and_skill_meshes_v1"
BLENDER_MINIMUM = (4, 5, 0)
FPS = 60
MASTER_CLIPS = {
    "idle": (0, 94),
    "run": (96, 158),
    "basic": (160, 222),
    "skillOne": (224, 302),
    "skillTwo": (304, 382),
    "ultimatePhase": (384, 478),
    "dashSpecial": (480, 526),
    "hurt": (528, 558),
    "death": (560, 654),
}

ACTORS = {
    "knight": {
        "category": "heroes",
        "source": "source/blender/heroes/knight_rigged.blend",
        "skeleton": "hero_humanoid",
        "display": "Knight",
    },
    "magic_caster": {
        "category": "heroes",
        "source": "source/blender/heroes/magic_caster_rigged.blend",
        "skeleton": "hero_humanoid",
        "display": "MagicCaster",
    },
    "riftling": {
        "category": "enemies",
        "source": "source/blender/enemies/riftling_rigged.blend",
        "skeleton": "quadruped",
        "display": "Riftling",
    },
    "hex_archer": {
        "category": "enemies",
        "source": "source/blender/enemies/hex_archer_rigged.blend",
        "skeleton": "hero_humanoid",
        "display": "HexArcher",
    },
    "obsidian_brute": {
        "category": "enemies",
        "source": "source/blender/enemies/obsidian_brute_rigged.blend",
        "skeleton": "humanoid",
        "display": "ObsidianBrute",
    },
    "brood_warden": {
        "category": "bosses",
        "source": "source/blender/bosses/brood_warden_rigged.blend",
        "skeleton": "quadruped",
        "display": "BroodWarden",
    },
    "hexeye_artillerist": {
        "category": "bosses",
        "source": "source/blender/bosses/hexeye_artillerist_rigged.blend",
        "skeleton": "floating_artillerist",
        "display": "HexeyeArtillerist",
    },
    "ironroot_colossus": {
        "category": "bosses",
        "source": "source/blender/bosses/ironroot_colossus_rigged.blend",
        "skeleton": "humanoid",
        "display": "IronrootColossus",
    },
    "eclipse_chimera": {
        "category": "bosses",
        "source": "source/blender/bosses/eclipse_chimera_rigged.blend",
        "skeleton": "chimera",
        "display": "EclipseChimera",
    },
    "void_sovereign": {
        "category": "bosses",
        "source": "source/blender/bosses/void_sovereign_rigged.blend",
        "skeleton": "humanoid",
        "display": "VoidSovereign",
    },
}

# Variants are derived from the approved production-v3 poses.  Only timing and
# small pose multipliers change, so character proportions, materials and the
# concept-approved silhouette remain untouched.
HERO_CLIPS = {
    "knight": [
        ("idle", "idle", {}),
        ("walk_forward", "run", {"timeScale": 1.45, "posePower": 0.72}),
        ("walk_backward", "run", {"timeScale": 1.45, "posePower": 0.72, "reverse": True}),
        ("run_forward", "run", {"upperBodyPower": 1.32}),
        ("run_backward", "run", {"reverse": True, "upperBodyPower": 1.32}),
        ("strafe_left", "run", {"upperBodyPower": 1.25, "lean": 0.12}),
        ("strafe_right", "run", {"upperBodyPower": 1.25, "lean": -0.12, "reverse": True}),
        ("basic_01", "basic", {"upperBodyPower": 1.10}),
        ("basic_02", "basic", {"upperBodyPower": 1.22, "reverse": True, "twistMirror": True}),
        ("basic_03", "basic", {"timeScale": 1.16, "upperBodyPower": 1.36}),
        ("skill_one", "skillOne", {}),
        ("skill_two", "skillTwo", {}),
        ("ultimate", "ultimatePhase", {}),
        ("dash", "dashSpecial", {}),
        ("hurt", "hurt", {}),
        ("death", "death", {}),
    ],
    "magic_caster": [
        ("idle", "idle", {}),
        ("walk_forward", "run", {"timeScale": 1.45, "posePower": 0.70}),
        ("walk_backward", "run", {"timeScale": 1.45, "posePower": 0.70, "reverse": True}),
        ("run_forward", "run", {"upperBodyPower": 1.28}),
        ("run_backward", "run", {"reverse": True, "upperBodyPower": 1.28}),
        ("strafe_left", "run", {"upperBodyPower": 1.22, "lean": 0.14}),
        ("strafe_right", "run", {"upperBodyPower": 1.22, "lean": -0.14, "reverse": True}),
        ("basic_01", "basic", {"upperBodyPower": 1.08}),
        ("basic_02", "basic", {"upperBodyPower": 1.14, "swapSides": True}),
        ("basic_03", "basic", {"timeScale": 1.10, "upperBodyPower": 1.28}),
        ("skill_one", "skillOne", {}),
        ("skill_two", "skillTwo", {}),
        ("ultimate", "ultimatePhase", {}),
        ("dash", "dashSpecial", {}),
        ("hurt", "hurt", {}),
        ("death", "death", {}),
    ],
}

HERO_RUNTIME_ACTIONS = [
    "idle", "walk_forward", "walk_backward", "run_forward",
    "strafe_left", "strafe_right", "basic_01", "basic_02", "basic_03",
    "skill_one", "skill_two", "ultimate", "dash", "hurt", "death",
]
NONHERO_RUNTIME_ACTIONS = [
    "idle", "run_forward", "basic_01", "skill_one", "skill_two",
    "ultimate", "special", "hurt", "death",
]

SKILLS = [
    {
        "id": "knight_violet_edge", "hero": "knight", "slot": "basic", "control": "J/LMB",
        "name": "Violet Edge", "characterAction": "basic_01",
        "comboActions": ["basic_01", "basic_02", "basic_03"],
        "kind": "weapon_arc_mesh", "space": "weapon_socket.R", "duration": 0.62,
        "contactNormalized": 28.0 / 62.0, "builder": "violet_edge",
    },
    {
        "id": "knight_aegis_counter", "hero": "knight", "slot": "skillOne", "control": "K/RMB",
        "name": "Aegis Counter", "characterAction": "skill_one",
        "kind": "shield_prop", "space": "character_front", "duration": 0.95,
        "contactNormalized": 50.0 / 78.0, "builder": "aegis_counter",
    },
    {
        "id": "knight_shield_rush", "hero": "knight", "slot": "skillTwo", "control": "U/Q",
        "name": "Shield Rush", "characterAction": "skill_two",
        "kind": "ram_shield_prop", "space": "character_front", "duration": 0.98,
        "contactNormalized": 40.0 / 78.0, "builder": "shield_rush",
    },
    {
        "id": "knight_bastion_breaker", "hero": "knight", "slot": "ultimate", "control": "H/R",
        "name": "Bastion Breaker", "characterAction": "ultimate",
        "kind": "crater_debris_field", "space": "world_ground", "duration": 1.55,
        "contactNormalized": 64.0 / 94.0, "builder": "bastion_breaker",
    },
    {
        "id": "knight_steel_step", "hero": "knight", "slot": "dash", "control": "L/Space",
        "name": "Steel Step", "characterAction": "dash",
        "kind": "armored_step_marker", "space": "world_ground", "duration": 0.38,
        "contactNormalized": 25.0 / 46.0, "builder": "steel_step",
    },
    {
        "id": "mage_arc_bolt", "hero": "magic_caster", "slot": "basic", "control": "J/LMB",
        "name": "Arc Bolt", "characterAction": "basic_01",
        "comboActions": ["basic_01", "basic_02", "basic_03"],
        "kind": "crystal_projectile", "space": "world_projectile", "duration": 0.66,
        "contactNormalized": 28.0 / 62.0, "builder": "arc_bolt",
    },
    {
        "id": "mage_frost_ring", "hero": "magic_caster", "slot": "skillOne", "control": "K/RMB",
        "name": "Frost Ring", "characterAction": "skill_one",
        "kind": "ice_shard_field", "space": "world_ground", "duration": 1.08,
        "contactNormalized": 50.0 / 78.0, "builder": "frost_ring",
    },
    {
        "id": "mage_gravity_well", "hero": "magic_caster", "slot": "skillTwo", "control": "U/Q",
        "name": "Gravity Well", "characterAction": "skill_two",
        "kind": "orbital_field", "space": "world_ground", "duration": 1.16,
        "contactNormalized": 40.0 / 78.0, "builder": "gravity_well",
    },
    {
        "id": "mage_astral_tempest", "hero": "magic_caster", "slot": "ultimate", "control": "H/R",
        "name": "Astral Tempest", "characterAction": "ultimate",
        "kind": "astral_pylon_field", "space": "world_ground", "duration": 1.68,
        "contactNormalized": 64.0 / 94.0, "builder": "astral_tempest",
    },
    {
        "id": "mage_phase_blink", "hero": "magic_caster", "slot": "dash", "control": "L/Space",
        "name": "Phase Blink", "characterAction": "dash",
        "kind": "rift_portal_prop", "space": "world_character_origin", "duration": 0.42,
        "contactNormalized": 25.0 / 46.0, "builder": "phase_blink",
    },
]


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--actor", choices=["all", *ACTORS], default="all")
    parser.add_argument("--skip-actors", action="store_true")
    parser.add_argument("--skip-skills", action="store_true")
    parser.add_argument("--no-render", action="store_true")
    parser.add_argument("--validate-only", action="store_true")
    arguments = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    return parser.parse_args(arguments)


def rel(path):
    return os.path.relpath(path, ASSET_ROOT).replace("\\", "/")


def sha256(path):
    digest = hashlib.sha256()
    with open(path, "rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest().upper()


def ensure_parent(path):
    os.makedirs(os.path.dirname(path), exist_ok=True)


def write_json(path, value):
    ensure_parent(path)
    with open(path, "w", encoding="utf-8", newline="\n") as handle:
        json.dump(value, handle, indent=2, sort_keys=False)
        handle.write("\n")


def configure_scene(frame_end=60):
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    scene.render.fps = FPS
    scene.frame_start = 0
    scene.frame_end = frame_end
    scene.render.engine = "BLENDER_EEVEE_NEXT"
    scene.render.resolution_x = 512
    scene.render.resolution_y = 512
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.film_transparent = True
    scene.view_settings.view_transform = "AgX"
    scene.view_settings.look = "AgX - Medium High Contrast"
    scene["step7Pipeline"] = PIPELINE_ID
    scene["sampleRate"] = FPS


def select_only(objects, active=None):
    bpy.ops.object.mode_set(mode="OBJECT") if bpy.context.object and bpy.context.object.mode != "OBJECT" else None
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.hide_set(False)
        obj.hide_viewport = False
        obj.select_set(True)
    if active is not None:
        bpy.context.view_layer.objects.active = active


def find_master_action(rig, actor_id):
    candidates = [
        action for action in bpy.data.actions
        if "master" in action.name.lower() and action.frame_range[1] >= 650
    ]
    if rig.animation_data and rig.animation_data.action in candidates:
        return rig.animation_data.action
    if len(candidates) != 1:
        raise RuntimeError(f"{actor_id}: expected one 655-frame master action, found {[a.name for a in candidates]}")
    return candidates[0]


def bake_constraint_pose_if_needed(rig, master):
    constrained = [bone for bone in rig.pose.bones if bone.constraints]
    if not constrained:
        return False
    rig.animation_data_create()
    rig.animation_data.action = master
    select_only([rig], active=rig)
    bpy.ops.object.mode_set(mode="POSE")
    bpy.ops.pose.select_all(action="SELECT")
    bpy.ops.nla.bake(
        frame_start=0,
        frame_end=654,
        step=1,
        only_selected=False,
        visual_keying=True,
        clear_constraints=True,
        clear_parents=False,
        use_current_action=True,
        clean_curves=False,
        bake_types={"POSE"},
    )
    bpy.ops.object.mode_set(mode="OBJECT")
    return True


def remove_keys_outside(action, first, last, time_scale=1.0, reverse=False):
    for curve in list(action.fcurves):
        # Blender 4.5 invalidates all previously captured Keyframe proxies after
        # one removal.  Remove by live reverse index instead of iterating a
        # copied proxy list.
        for index in range(len(curve.keyframe_points) - 1, -1, -1):
            key = curve.keyframe_points[index]
            if key.co.x < first - 0.001 or key.co.x > last + 0.001:
                curve.keyframe_points.remove(key, fast=True)
        if not curve.keyframe_points:
            action.fcurves.remove(curve)
            continue
        for key in curve.keyframe_points:
            source_x = float(key.co.x)
            target_x = ((last - source_x) if reverse else (source_x - first)) * time_scale
            key.co.x = target_x
            key.handle_left_type = "AUTO_CLAMPED"
            key.handle_right_type = "AUTO_CLAMPED"
        curve.update()


def curve_is_pose_rotation(curve):
    return "pose.bones[" in curve.data_path and "rotation_" in curve.data_path


def multiply_pose(action, factor, upper_only=False):
    upper_tokens = ("chest", "head", "upper_arm", "forearm", "hand", "weapon_socket")
    for curve in action.fcurves:
        if not curve_is_pose_rotation(curve):
            continue
        if upper_only and not any(token in curve.data_path for token in upper_tokens):
            continue
        for key in curve.keyframe_points:
            key.co.y *= factor
            key.handle_left.y = key.co.y
            key.handle_right.y = key.co.y


def add_strafe_lean(action, lean):
    for curve in action.fcurves:
        if curve.array_index != 1 or "rotation_euler" not in curve.data_path:
            continue
        if not any(token in curve.data_path for token in ('["hips"]', '["chest"]')):
            continue
        amount = lean if '["chest"]' in curve.data_path else lean * 0.55
        for key in curve.keyframe_points:
            key.co.y += amount
            key.handle_left.y = key.co.y
            key.handle_right.y = key.co.y


def mirror_twist(action):
    for curve in action.fcurves:
        if curve.array_index == 2 and curve_is_pose_rotation(curve):
            for key in curve.keyframe_points:
                key.co.y = -key.co.y
                key.handle_left.y = key.co.y
                key.handle_right.y = key.co.y


def swap_sides(action):
    # Mirror the casting arms only.  The modular staff socket deliberately
    # exists on the right hand; renaming weapon_socket.R would target a bone
    # that is not part of the approved 16-joint contract.
    arm_curves = [
        curve for curve in action.fcurves
        if any(token in curve.data_path for token in ("upper_arm", "forearm", "hand"))
        and "weapon_socket" not in curve.data_path
    ]
    for curve in arm_curves:
        curve.data_path = curve.data_path.replace(".L\"]", ".STEP7_TMP\"]")
    for curve in arm_curves:
        curve.data_path = curve.data_path.replace(".R\"]", ".L\"]")
    for curve in arm_curves:
        curve.data_path = curve.data_path.replace(".STEP7_TMP\"]", ".R\"]")


def make_action(master, name, semantic, options):
    first, last = MASTER_CLIPS[semantic]
    action = master.copy()
    action.name = name
    action.use_fake_user = True
    action["step7Semantic"] = semantic
    action["sourceFrameStart"] = first
    action["sourceFrameEnd"] = last
    action["sampleRate"] = FPS
    remove_keys_outside(
        action, first, last,
        time_scale=float(options.get("timeScale", 1.0)),
        reverse=bool(options.get("reverse", False)),
    )
    if "posePower" in options:
        multiply_pose(action, float(options["posePower"]), upper_only=False)
    if "upperBodyPower" in options:
        multiply_pose(action, float(options["upperBodyPower"]), upper_only=True)
    if "lean" in options:
        add_strafe_lean(action, float(options["lean"]))
    if options.get("twistMirror"):
        mirror_twist(action)
    if options.get("swapSides"):
        swap_sides(action)
    return action


def actor_specs(actor_id, display):
    if actor_id in HERO_CLIPS:
        return HERO_CLIPS[actor_id]
    return [
        ("idle", "idle", {}),
        ("run_forward", "run", {}),
        ("basic_01", "basic", {}),
        ("skill_one", "skillOne", {}),
        ("skill_two", "skillTwo", {}),
        ("ultimate", "ultimatePhase", {}),
        ("special", "dashSpecial", {}),
        ("hurt", "hurt", {}),
        ("death", "death", {}),
    ]


def export_gltf(path, selected, active, animations=True):
    ensure_parent(path)
    select_only(selected, active=active)
    bpy.ops.export_scene.gltf(
        filepath=path,
        export_format="GLB",
        use_selection=True,
        export_yup=True,
        export_animations=animations,
        export_animation_mode="ACTIONS",
        export_frame_range=False,
        export_force_sampling=True,
        export_frame_step=1,
        export_anim_slide_to_zero=True,
        export_skins=True,
        export_def_bones=False,
        export_leaf_bone=False,
        export_all_influences=False,
        export_influence_nb=4,
        export_morph=False,
        export_materials="EXPORT",
        export_cameras=False,
        export_lights=False,
        export_extras=True,
        export_optimize_animation_size=True,
        export_optimize_animation_keep_anim_armature=True,
        export_extra_animations=True,
    )


def build_actor(actor_id):
    definition = ACTORS[actor_id]
    source = os.path.join(ASSET_ROOT, definition["source"].replace("/", os.sep))
    if not os.path.isfile(source):
        raise RuntimeError(f"Missing actor source: {source}")
    bpy.ops.wm.open_mainfile(filepath=source)
    configure_scene(654)
    rigs = [obj for obj in bpy.context.scene.objects if obj.type == "ARMATURE"]
    meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    if len(rigs) != 1 or not meshes:
        raise RuntimeError(f"{actor_id}: expected one rig and at least one mesh")
    rig = rigs[0]
    master = find_master_action(rig, actor_id)
    constraint_baked = bake_constraint_pose_if_needed(rig, master)

    rig.animation_data_create()
    rig.animation_data.action = master
    for obj in list(bpy.context.scene.objects):
        if obj != rig and obj.animation_data:
            obj.animation_data_clear()
    for action in list(bpy.data.actions):
        if action != master:
            bpy.data.actions.remove(action)

    actions = []
    specs = actor_specs(actor_id, definition["display"])
    for action_name, semantic, options in specs:
        actions.append(make_action(master, action_name, semantic, options))
    rig.animation_data.action = actions[0]
    bpy.data.actions.remove(master)
    for track in list(rig.animation_data.nla_tracks):
        rig.animation_data.nla_tracks.remove(track)

    scene = bpy.context.scene
    scene.frame_start = 0
    scene.frame_end = max(int(action.frame_range[1]) for action in actions)
    scene["namedActionContract"] = "Step7"
    scene["actorId"] = actor_id
    rig["actorId"] = actor_id
    rig["skeletonContract"] = definition["skeleton"]

    blend_path = os.path.join(ACTOR_BLEND_ROOT, f"{actor_id}_named_actions.blend")
    glb_path = os.path.join(ACTOR_MODEL_ROOT, f"{actor_id}_named_actions.glb")
    ensure_parent(blend_path)
    bpy.context.preferences.filepaths.save_version = 0
    bpy.ops.wm.save_as_mainfile(filepath=blend_path, check_existing=False)
    export_gltf(glb_path, [*meshes, rig], rig, animations=True)
    return {
        "id": actor_id,
        "category": definition["category"],
        "skeletonContract": definition["skeleton"],
        "sourceMaster": definition["source"],
        "editableBlend": rel(blend_path),
        "model": rel(glb_path),
        "constraintBakeApplied": constraint_baked,
        "namedActions": [action.name for action in actions],
        "actionCount": len(actions),
        "locomotionExpanded": actor_id in HERO_CLIPS,
        "comboExpanded": actor_id in HERO_CLIPS,
    }


def material(name, color, metallic=0.0, roughness=0.4, emission=0.0):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    mat.diffuse_color = (*color, 1.0)
    node = mat.node_tree.nodes.get("Principled BSDF")
    if node:
        node.inputs["Base Color"].default_value = (*color, 1.0)
        node.inputs["Metallic"].default_value = metallic
        node.inputs["Roughness"].default_value = roughness
        if "Emission Color" in node.inputs:
            node.inputs["Emission Color"].default_value = (*color, 1.0)
            node.inputs["Emission Strength"].default_value = emission
    return mat


def assign_material(obj, mat):
    obj.data.materials.append(mat)
    return obj


def smooth(obj):
    if obj.type == "MESH":
        for polygon in obj.data.polygons:
            polygon.use_smooth = True
    return obj


def bevel(obj, width=0.035, segments=2):
    modifier = obj.modifiers.new("ProductionBevel", "BEVEL")
    modifier.width = width
    modifier.segments = segments
    modifier.limit_method = "ANGLE"
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    return obj


def add_cube(name, location, scale, mat, rotation=(0.0, 0.0, 0.0), bevel_width=0.03):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=location, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    obj.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    assign_material(obj, mat)
    # Do not use Blender's threaded Bevel modifier here.  Its generated vertex
    # order is visually stable but not byte-stable between background runs,
    # which makes reproducible GLB hashes impossible.  Prism assets use the
    # deterministic custom extrusion above; boxes intentionally keep the
    # project's low-poly hard-surface silhouette.
    _ = bevel_width
    return obj


def add_cone(name, location, radius1, radius2, depth, mat, vertices=8,
             rotation=(0.0, 0.0, 0.0)):
    bpy.ops.mesh.primitive_cone_add(
        vertices=vertices, radius1=radius1, radius2=radius2, depth=depth,
        location=location, rotation=rotation,
    )
    obj = bpy.context.object
    obj.name = name
    assign_material(obj, mat)
    return smooth(obj)


def add_ico(name, location, radius, scale, mat, subdivisions=2):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=subdivisions, radius=radius, location=location)
    obj = bpy.context.object
    obj.name = name
    obj.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    assign_material(obj, mat)
    return smooth(obj)


def add_torus(name, location, major_radius, minor_radius, mat,
              rotation=(0.0, 0.0, 0.0), major_segments=32, minor_segments=8):
    bpy.ops.mesh.primitive_torus_add(
        align="WORLD", major_segments=major_segments, minor_segments=minor_segments,
        major_radius=major_radius, minor_radius=minor_radius,
        location=location, rotation=rotation,
    )
    obj = bpy.context.object
    obj.name = name
    assign_material(obj, mat)
    return smooth(obj)


def prism(name, points_xz, depth, mat, center_y=0.0):
    count = len(points_xz)
    vertices = [(x, center_y - depth * 0.5, z) for x, z in points_xz]
    vertices += [(x, center_y + depth * 0.5, z) for x, z in points_xz]
    faces = [tuple(range(count - 1, -1, -1)), tuple(range(count, count * 2))]
    for index in range(count):
        next_index = (index + 1) % count
        faces.append((index, next_index, count + next_index, count + index))
    mesh = bpy.data.meshes.new(name + "Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    assign_material(obj, mat)
    bevel(obj, min(0.025, depth * 0.18), 2)
    return obj


def crescent_points(outer, inner, start=-1.18, end=1.18, segments=24, z_offset=0.75):
    outer_points = []
    inner_points = []
    for index in range(segments + 1):
        angle = start + (end - start) * index / segments
        outer_points.append((math.sin(angle) * outer, z_offset + math.cos(angle) * outer))
    for index in range(segments, -1, -1):
        angle = start + (end - start) * index / segments
        taper = max(0.30, math.sin((index / segments) * math.pi))
        radius = inner + (outer - inner) * (1.0 - taper) * 0.10
        inner_points.append((math.sin(angle) * radius, z_offset + math.cos(angle) * radius))
    return outer_points + inner_points


def palette():
    return {
        "violet": material("VioletEnergy", (0.38, 0.09, 0.72), 0.18, 0.26, 2.2),
        "purple": material("RoyalPurple", (0.19, 0.04, 0.36), 0.35, 0.30, 0.7),
        "gold": material("KnightGold", (0.92, 0.56, 0.12), 0.78, 0.22, 0.35),
        "silver": material("KnightSilver", (0.52, 0.60, 0.72), 0.86, 0.18, 0.18),
        "stone": material("RiftStone", (0.12, 0.11, 0.17), 0.05, 0.86, 0.0),
        "cyan": material("ArcaneCyan", (0.05, 0.52, 0.92), 0.12, 0.20, 2.8),
        "ice": material("FrostCrystal", (0.28, 0.82, 1.0), 0.05, 0.12, 2.0),
        "indigo": material("GravityIndigo", (0.08, 0.04, 0.30), 0.28, 0.22, 1.2),
        "astral": material("AstralWhite", (0.72, 0.88, 1.0), 0.20, 0.14, 3.2),
    }


def build_violet_edge(mats):
    prism("VioletEdgeArc", crescent_points(1.55, 1.18), 0.16, mats["violet"], -0.02)
    for i, angle in enumerate((-0.72, 0.0, 0.72)):
        x = math.sin(angle) * 1.72
        z = 0.75 + math.cos(angle) * 1.72
        add_cone(f"EdgeShard{i}", (x, 0.0, z), 0.10, 0.0, 0.42,
                 mats["gold"], 6, rotation=(0.0, angle, angle * 0.2))


def build_aegis_counter(mats):
    outline = [(-0.72, 0.45), (-0.62, 1.38), (0.0, 1.82), (0.62, 1.38), (0.72, 0.45), (0.0, 0.05)]
    inset = [(x * 0.76, 0.32 + (z - 0.32) * 0.76) for x, z in outline]
    prism("AegisOuter", outline, 0.22, mats["silver"], 0.0)
    prism("AegisFace", inset, 0.25, mats["purple"], -0.10)
    add_ico("AegisCore", (0.0, -0.24, 0.92), 0.22, (1.0, 0.45, 1.0), mats["gold"], 1)
    add_torus("AegisHalo", (0.0, 0.03, 0.94), 0.92, 0.035, mats["violet"], rotation=(math.pi / 2, 0, 0), major_segments=28)


def build_shield_rush(mats):
    outline = [(-0.68, 0.12), (-0.70, 1.58), (-0.42, 1.82), (0.42, 1.82), (0.70, 1.58), (0.68, 0.12)]
    prism("RushTowerShield", outline, 0.34, mats["silver"], 0.0)
    inset = [(x * 0.78, 0.22 + (z - 0.22) * 0.80) for x, z in outline]
    prism("RushVioletFace", inset, 0.37, mats["violet"], -0.12)
    for index, x in enumerate((-0.42, 0.0, 0.42)):
        add_cone(f"RushSpike{index}", (x, -0.39, 0.82), 0.12, 0.0, 0.58,
                 mats["gold"], 8, rotation=(math.pi / 2, 0.0, 0.0))
    add_cube("RushBrace", (0.0, 0.23, 0.92), (0.54, 0.08, 0.10), mats["gold"], bevel_width=0.025)


def build_bastion_breaker(mats):
    add_torus("BastionCraterOuter", (0.0, 0.0, 0.10), 2.05, 0.16, mats["stone"], major_segments=40)
    add_torus("BastionCraterRune", (0.0, 0.0, 0.13), 1.42, 0.055, mats["violet"], major_segments=36)
    for index in range(14):
        angle = index * math.tau / 14.0
        radius = 1.58 + (index % 3) * 0.20
        size = 0.22 + (index % 4) * 0.045
        add_cube(
            f"BastionDebris{index}",
            (math.cos(angle) * radius, math.sin(angle) * radius, 0.18 + size * 0.35),
            (size, size * 0.72, size * 0.55), mats["stone"],
            rotation=(angle * 0.17, angle * 0.11, angle), bevel_width=0.025,
        )
    add_ico("BastionImpactCore", (0.0, 0.0, 0.22), 0.46, (1.0, 1.0, 0.32), mats["gold"], 2)


def build_steel_step(mats):
    for index, x in enumerate((-0.22, 0.22)):
        add_cube(f"SteelSole{index}", (x, -0.12, 0.12), (0.17, 0.44, 0.10), mats["silver"],
                 rotation=(0.0, 0.0, (-0.08 if index == 0 else 0.08)), bevel_width=0.055)
        add_cube(f"SteelHeel{index}", (x, 0.22, 0.25), (0.16, 0.15, 0.18), mats["gold"], bevel_width=0.035)
    for index in range(6):
        length = 0.28 + index * 0.10
        add_cone(f"StepWake{index}", ((index - 2.5) * 0.18, 0.72 + index * 0.18, 0.10),
                 0.09, 0.0, length, mats["violet"], 6,
                 rotation=(math.pi / 2, 0.0, 0.0))


def build_arc_bolt(mats):
    add_ico("ArcBoltCore", (0.0, 0.0, 0.55), 0.48, (0.70, 1.65, 0.70), mats["cyan"], 2)
    add_cone("ArcBoltTip", (0.0, -0.92, 0.55), 0.30, 0.0, 0.85, mats["astral"], 8,
             rotation=(math.pi / 2, 0.0, 0.0))
    add_torus("ArcBoltRing", (0.0, 0.18, 0.55), 0.48, 0.055, mats["violet"],
              rotation=(math.pi / 2, 0.0, 0.0), major_segments=24)
    for index, angle in enumerate((0.0, math.pi * 0.5, math.pi, math.pi * 1.5)):
        add_cone(f"BoltFin{index}", (math.cos(angle) * 0.40, 0.22, 0.55 + math.sin(angle) * 0.40),
                 0.10, 0.0, 0.42, mats["ice"], 6, rotation=(0.0, angle, angle))


def build_frost_ring(mats):
    add_torus("FrostRingOuter", (0.0, 0.0, 0.09), 2.15, 0.12, mats["ice"], major_segments=40)
    add_torus("FrostRingInner", (0.0, 0.0, 0.11), 1.62, 0.045, mats["cyan"], major_segments=36)
    for index in range(16):
        angle = index * math.tau / 16.0
        radius = 1.96
        height = 0.48 + (index % 4) * 0.18
        add_cone(f"FrostShard{index}", (math.cos(angle) * radius, math.sin(angle) * radius, height * 0.5),
                 0.13 + (index % 2) * 0.035, 0.0, height, mats["ice"], 6,
                 rotation=(0.12 * math.cos(angle), 0.12 * math.sin(angle), angle))


def build_gravity_well(mats):
    add_ico("GravityCore", (0.0, 0.0, 0.68), 0.58, (1.0, 1.0, 1.0), mats["indigo"], 3)
    add_ico("GravityHeart", (0.0, 0.0, 0.68), 0.26, (1.0, 1.0, 1.0), mats["astral"], 2)
    rotations = [(0.0, 0.0, 0.0), (math.radians(58), 0.0, math.radians(22)),
                 (math.radians(-58), 0.0, math.radians(-22))]
    for index, rotation in enumerate(rotations):
        add_torus(f"GravityOrbit{index}", (0.0, 0.0, 0.68), 1.05 + index * 0.27,
                  0.07 - index * 0.01, mats["violet" if index != 1 else "cyan"],
                  rotation=rotation, major_segments=36)
    add_torus("GravityGroundSeal", (0.0, 0.0, 0.08), 1.75, 0.045, mats["cyan"], major_segments=40)


def build_astral_tempest(mats):
    add_ico("AstralCore", (0.0, 0.0, 1.05), 0.46, (1.0, 1.0, 1.35), mats["astral"], 2)
    add_torus("AstralCrown", (0.0, 0.0, 1.05), 0.84, 0.06, mats["cyan"],
              rotation=(math.pi / 2, 0.0, 0.0), major_segments=30)
    for index in range(8):
        angle = index * math.tau / 8.0
        radius = 1.72 + (index % 2) * 0.28
        height = 1.10 + (index % 3) * 0.30
        add_cone(f"AstralPylon{index}", (math.cos(angle) * radius, math.sin(angle) * radius, height * 0.5),
                 0.18, 0.055, height, mats["violet" if index % 2 else "cyan"], 6,
                 rotation=(0.08 * math.sin(angle), 0.08 * math.cos(angle), angle))
        add_ico(f"AstralStar{index}", (math.cos(angle) * radius, math.sin(angle) * radius, height + 0.18),
                0.18, (1.0, 1.0, 1.35), mats["astral"], 1)
    add_torus("AstralGroundOrbit", (0.0, 0.0, 0.08), 2.22, 0.055, mats["violet"], major_segments=44)


def build_phase_blink(mats):
    for index, radius in enumerate((0.82, 1.08, 1.30)):
        add_torus(f"BlinkPortalRing{index}", (0.0, 0.0, 1.12), radius, 0.065 - index * 0.012,
                  mats["cyan" if index == 1 else "violet"],
                  rotation=(math.pi / 2, 0.0, index * 0.18), major_segments=36)
    for index in range(10):
        angle = index * math.tau / 10.0
        add_cone(f"BlinkShard{index}", (math.cos(angle) * 1.30, -0.04, 1.12 + math.sin(angle) * 1.30),
                 0.09, 0.0, 0.38 + (index % 3) * 0.08,
                 mats["astral"], 6, rotation=(angle, 0.0, math.pi / 2))
    add_ico("BlinkCore", (0.0, 0.0, 1.12), 0.36, (0.42, 0.20, 1.45), mats["indigo"], 2)


SKILL_BUILDERS = {
    "violet_edge": build_violet_edge,
    "aegis_counter": build_aegis_counter,
    "shield_rush": build_shield_rush,
    "bastion_breaker": build_bastion_breaker,
    "steel_step": build_steel_step,
    "arc_bolt": build_arc_bolt,
    "frost_ring": build_frost_ring,
    "gravity_well": build_gravity_well,
    "astral_tempest": build_astral_tempest,
    "phase_blink": build_phase_blink,
}

SKILL_VFX = {
    "knight_violet_edge": "knight_sword_arc",
    "knight_aegis_counter": "knight_guard_crest",
    "knight_shield_rush": "knight_shield_rush_impact",
    "knight_bastion_breaker": "knight_blade_storm",
    "knight_steel_step": "hero_dash_streak",
    "mage_arc_bolt": "mage_arcane_bolt",
    "mage_frost_ring": "mage_frost_nova",
    "mage_gravity_well": "mage_gravity_vortex",
    "mage_astral_tempest": "mage_astral_tempest",
    "mage_phase_blink": "hero_dash_streak",
}


def join_skill_meshes(skill_id):
    meshes = sorted([obj for obj in bpy.context.scene.objects if obj.type == "MESH"], key=lambda obj: obj.name)
    if not meshes:
        raise RuntimeError(f"{skill_id}: builder produced no meshes")
    select_only(meshes, active=meshes[0])
    bpy.ops.object.join()
    mesh = bpy.context.object
    mesh.name = skill_id + "_mesh"
    mesh.data.name = skill_id + "_geometry"
    return mesh


def rig_skill_mesh(skill_id, mesh):
    armature = bpy.data.armatures.new(skill_id + "_armature")
    rig = bpy.data.objects.new(skill_id + "_rig", armature)
    bpy.context.collection.objects.link(rig)
    bpy.context.view_layer.objects.active = rig
    rig.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    bone = armature.edit_bones.new("root")
    bone.head = (0.0, 0.0, 0.0)
    bone.tail = (0.0, 0.0, 1.0)
    bone.use_deform = True
    bpy.ops.object.mode_set(mode="OBJECT")
    mesh.parent = rig
    modifier = mesh.modifiers.new("SkillArmature", "ARMATURE")
    modifier.object = rig
    group = mesh.vertex_groups.new(name="root")
    group.add(list(range(len(mesh.data.vertices))), 1.0, "REPLACE")

    rig.animation_data_create()
    # Each skill lives in its own GLB, so a compact shared clip name is clearer
    # and safely fits raylib's fixed 32-byte ModelAnimation name buffer.
    action = bpy.data.actions.new("Activate")
    action.use_fake_user = True
    rig.animation_data.action = action
    root = rig.pose.bones["root"]
    root.rotation_mode = "XYZ"
    poses = [
        (0, (0.22, 0.22, 0.22), (0.0, 0.0, -0.16), (0.0, 0.0, -0.08)),
        (18, (0.78, 0.78, 0.78), (0.0, 0.0, 0.10), (0.0, 0.0, 0.03)),
        (34, (1.08, 1.08, 1.08), (0.0, 0.0, 0.28), (0.0, 0.0, 0.0)),
        (60, (1.0, 1.0, 1.0), (0.0, 0.0, 0.44), (0.0, 0.0, 0.0)),
    ]
    for frame, scale, rotation, location in poses:
        root.scale = scale
        root.rotation_euler = rotation
        root.location = location
        root.keyframe_insert("scale", frame=frame, group="root")
        root.keyframe_insert("rotation_euler", frame=frame, group="root")
        root.keyframe_insert("location", frame=frame, group="root")
    for curve in action.fcurves:
        for key in curve.keyframe_points:
            key.interpolation = "BEZIER"
            key.handle_left_type = "AUTO_CLAMPED"
            key.handle_right_type = "AUTO_CLAMPED"
    action["skillId"] = skill_id
    action["sampleRate"] = FPS
    rig["skillId"] = skill_id
    rig["assetContract"] = "Step7SkillMesh"
    return rig, action


def look_at(camera, target):
    camera.rotation_euler = (Vector(target) - camera.location).to_track_quat("-Z", "Y").to_euler()


def render_skill_qa(skill, mesh, rig, path):
    scene = bpy.context.scene
    camera_data = bpy.data.cameras.new("Step7QaCamera")
    camera = bpy.data.objects.new("Step7QaCamera", camera_data)
    bpy.context.collection.objects.link(camera)
    corners = [mesh.matrix_world @ Vector(corner) for corner in mesh.bound_box]
    minimum = Vector((min(point.x for point in corners), min(point.y for point in corners), min(point.z for point in corners)))
    maximum = Vector((max(point.x for point in corners), max(point.y for point in corners), max(point.z for point in corners)))
    center = (minimum + maximum) * 0.5
    extent = max(0.8, max(maximum - minimum))
    camera_direction = Vector((0.82, -1.16, 0.86)).normalized()
    camera.location = center + camera_direction * (extent * 2.35)
    camera_data.lens = 54
    look_at(camera, center)
    scene.camera = camera
    world = bpy.data.worlds.new("Step7QaWorld") if scene.world is None else scene.world
    scene.world = world
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.012, 0.008, 0.025, 1.0)
    world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.22
    for index, (location, energy, color) in enumerate([
        ((4.0, -3.0, 6.0), 1100.0, (0.70, 0.78, 1.0)),
        ((-3.0, -1.0, 3.0), 750.0, (0.50, 0.20, 1.0)),
    ]):
        light_data = bpy.data.lights.new(f"Step7QaLight{index}", "AREA")
        light_data.energy = energy
        light_data.color = color
        light_data.shape = "DISK"
        light_data.size = 4.0
        light = bpy.data.objects.new(light_data.name, light_data)
        light.location = location
        look_at(light, (0.0, 0.0, 0.8))
        bpy.context.collection.objects.link(light)
    scene.frame_set(60)
    ensure_parent(path)
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)


def build_skill(skill, no_render=False):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    configure_scene(60)
    mats = palette()
    SKILL_BUILDERS[skill["builder"]](mats)
    mesh = join_skill_meshes(skill["id"])
    rig, action = rig_skill_mesh(skill["id"], mesh)
    skill["assetAnimation"] = action.name
    scene = bpy.context.scene
    scene["skillId"] = skill["id"]
    scene["hero"] = skill["hero"]
    scene["slot"] = skill["slot"]
    scene["characterAction"] = skill["characterAction"]

    qa_path = os.path.join(QA_ROOT, skill["id"] + ".png")
    if not no_render:
        render_skill_qa(skill, mesh, rig, qa_path)
    # Export the bind/rest presentation, independent of whether QA rendering
    # moved the scene to the final activation frame.
    scene.frame_set(0)
    blend_path = os.path.join(SKILL_BLEND_ROOT, skill["id"] + ".blend")
    glb_path = os.path.join(SKILL_MODEL_ROOT, skill["id"] + ".glb")
    ensure_parent(blend_path)
    bpy.context.preferences.filepaths.save_version = 0
    bpy.ops.wm.save_as_mainfile(filepath=blend_path, check_existing=False)
    export_gltf(glb_path, [mesh, rig], rig, animations=True)
    return {
        **{key: value for key, value in skill.items() if key != "builder"},
        "model": rel(glb_path),
        "editableBlend": rel(blend_path),
        "qaRender": rel(qa_path) if os.path.isfile(qa_path) else None,
        "materialCount": len(mesh.data.materials),
        "vertexCount": len(mesh.data.vertices),
        "triangleCount": sum(max(0, len(poly.vertices) - 2) for poly in mesh.data.polygons),
        "boneCount": len(rig.data.bones),
        "collisionProxy": "oriented_box" if "projectile" in skill["kind"] else "radius_from_manifest",
        "flatTexturePlane": False,
    }


def read_glb(path):
    with open(path, "rb") as handle:
        header = handle.read(12)
        if len(header) != 12:
            raise RuntimeError(f"Truncated GLB: {path}")
        magic, version, length = struct.unpack("<4sII", header)
        if magic != b"glTF" or version != 2 or length != os.path.getsize(path):
            raise RuntimeError(f"Invalid GLB header: {path}")
        chunk_length, chunk_type = struct.unpack("<II", handle.read(8))
        if chunk_type != 0x4E4F534A:
            raise RuntimeError(f"Missing GLB JSON chunk: {path}")
        return json.loads(handle.read(chunk_length).decode("utf-8").rstrip(" \t\r\n\0"))


def glb_bounds(document):
    minimum = [float("inf")] * 3
    maximum = [float("-inf")] * 3
    found = False
    accessors = document.get("accessors", [])
    for mesh in document.get("meshes", []):
        for primitive in mesh.get("primitives", []):
            position_index = primitive.get("attributes", {}).get("POSITION")
            if position_index is None:
                continue
            accessor = accessors[position_index]
            if "min" not in accessor or "max" not in accessor:
                continue
            found = True
            for axis in range(3):
                minimum[axis] = min(minimum[axis], accessor["min"][axis])
                maximum[axis] = max(maximum[axis], accessor["max"][axis])
    if not found:
        raise RuntimeError("GLB has no bounded POSITION accessor")
    return minimum, maximum


def validate_actor(result):
    path = os.path.join(ASSET_ROOT, result["model"].replace("/", os.sep))
    document = read_glb(path)
    names = [animation.get("name", "") for animation in document.get("animations", [])]
    required = result["namedActions"]
    missing = sorted(set(required) - set(names))
    unexpected = sorted(set(names) - set(required))
    if missing or unexpected:
        raise RuntimeError(f"{result['id']}: animation mismatch missing={missing} unexpected={unexpected}")
    if not document.get("meshes") or not document.get("skins"):
        raise RuntimeError(f"{result['id']}: named-action GLB lacks mesh/skin")
    if len(names) != result["actionCount"]:
        raise RuntimeError(f"{result['id']}: duplicate or missing exported animations")
    runtime_required = HERO_RUNTIME_ACTIONS if result["id"] in HERO_CLIPS else NONHERO_RUNTIME_ACTIONS
    runtime_missing = sorted(set(runtime_required) - set(names))
    if runtime_missing:
        raise RuntimeError(f"{result['id']}: runtime contract missing={runtime_missing}")
    result["glbAudit"] = {
        "animationNames": names,
        "animationCount": len(names),
        "meshCount": len(document.get("meshes", [])),
        "skinCount": len(document.get("skins", [])),
        "sha256": sha256(path),
    }
    result["blendSha256"] = sha256(os.path.join(ASSET_ROOT, result["editableBlend"].replace("/", os.sep)))


def validate_skill(result):
    path = os.path.join(ASSET_ROOT, result["model"].replace("/", os.sep))
    document = read_glb(path)
    names = [animation.get("name", "") for animation in document.get("animations", [])]
    if result["assetAnimation"] not in names:
        raise RuntimeError(f"{result['id']}: missing asset animation {result['assetAnimation']}; got {names}")
    if not document.get("meshes") or not document.get("materials") or not document.get("skins"):
        raise RuntimeError(f"{result['id']}: requires mesh, materials and root-bone skin")
    minimum, maximum = glb_bounds(document)
    dimensions = [maximum[index] - minimum[index] for index in range(3)]
    if min(dimensions) < 0.08:
        raise RuntimeError(f"{result['id']}: asset is effectively flat, dimensions={dimensions}")
    if result["triangleCount"] < 48 or result["vertexCount"] < 32:
        raise RuntimeError(f"{result['id']}: insufficient production geometry")
    result["glbAudit"] = {
        "animationNames": names,
        "meshCount": len(document.get("meshes", [])),
        "materialCount": len(document.get("materials", [])),
        "skinCount": len(document.get("skins", [])),
        "boundsMeters": [round(value, 5) for value in dimensions],
        "sha256": sha256(path),
    }
    result["blendSha256"] = sha256(os.path.join(ASSET_ROOT, result["editableBlend"].replace("/", os.sep)))


def existing_results_for_validation():
    if not os.path.isfile(STEP7_MANIFEST):
        raise RuntimeError("Step 7 manifest is missing; build before --validate-only")
    manifest = json.load(open(STEP7_MANIFEST, encoding="utf-8"))
    return manifest["actors"], manifest["skills"]


def write_manifests(actor_results, skill_results, no_render):
    animation_document = {
        "schemaVersion": 1,
        "pipeline": PIPELINE_ID,
        "format": "glb",
        "sampleRate": FPS,
        "legacyRuntimeMasterTimelinePreserved": True,
        "namedActionRuntimeCandidates": actor_results,
        "heroContracts": {
            "knight": {
                "requiredRuntimeActions": HERO_RUNTIME_ACTIONS,
                "directionalAliases": ["run_backward"],
                "locomotion": ["idle", "walk_forward", "walk_backward", "run_forward", "run_backward", "strafe_left", "strafe_right"],
                "basicCombo": ["basic_01", "basic_02", "basic_03"],
            },
            "magic_caster": {
                "requiredRuntimeActions": HERO_RUNTIME_ACTIONS,
                "directionalAliases": ["run_backward"],
                "locomotion": ["idle", "walk_forward", "walk_backward", "run_forward", "run_backward", "strafe_left", "strafe_right"],
                "basicCombo": ["basic_01", "basic_02", "basic_03"],
            },
        },
        "nonHeroContract": {
            "requiredRuntimeActions": NONHERO_RUNTIME_ACTIONS,
        },
    }
    runtime_skill_entries = []
    for item in skill_results:
        runtime_skill_entries.append({
            "id": item["id"],
            "actor": item["hero"],
            "slot": item["slot"],
            "model": item["model"],
            "animationClip": item["characterAction"],
            "assetAnimationClip": item["assetAnimation"],
            "vfx": SKILL_VFX[item["id"]],
            "eventTrack": [
                {"event": "SpawnSkillModel", "normalizedTime": 0.0},
                {"event": "CombatContact", "normalizedTime": round(item["contactNormalized"], 6)},
                {"event": "DespawnSkillModel", "normalizedTime": 1.0},
            ],
            "geometryType": item["kind"],
            "runtimeRequired": True,
            "generatedSha256": item["glbAudit"]["sha256"],
            "space": item["space"],
            "durationSeconds": item["duration"],
            "editableBlend": item["editableBlend"],
            "flatTexturePlane": False,
        })
    skill_document = {
        "schemaVersion": 1,
        "pipeline": PIPELINE_ID,
        "runtime": "C++17/raylib",
        "coordinateContract": "+Z up, -Y forward, meters",
        "renderContract": "real triangle mesh with embedded PBR materials; never a textured plane",
        "skills": runtime_skill_entries,
    }
    write_json(ANIMATION_MANIFEST, animation_document)
    write_json(SKILL_MANIFEST, skill_document)
    step7_document = {
        "schemaVersion": 1,
        "pipeline": PIPELINE_ID,
        "status": "asset_scope_complete",
        "actors": actor_results,
        "skills": skill_results,
        "validation": {
            "actorCount": len(actor_results),
            "skillCount": len(skill_results),
            "actorNamedActionCount": sum(item["actionCount"] for item in actor_results),
            "skillModelsWithReal3DGeometry": sum(not item["flatTexturePlane"] for item in skill_results),
            "qaRenderCount": sum(bool(item.get("qaRender")) for item in skill_results),
            "heroRequiredRuntimeActions": HERO_RUNTIME_ACTIONS,
            "nonHeroRequiredRuntimeActions": NONHERO_RUNTIME_ACTIONS,
        },
    }
    write_json(STEP7_MANIFEST, step7_document)
    report = {
        "schemaVersion": 1,
        "milestone": "Step 7 - Named Animation Clips and Non-VFX 3D Skill Assets",
        "status": "asset_scope_complete",
        "runtimeLanguage": "C++17",
        "offlineAuthoring": f"Blender {bpy.app.version_string}",
        "survivalRuntimePythonFilesAdded": 0,
        "pipelineScript": "tools/build_step7_animation_skill_assets.py",
        "conceptAppearancePreserved": True,
        "legacyRuntimeModelsOverwritten": False,
        "actorCount": len(actor_results),
        "skillModelCount": len(skill_results),
        "allTenActorsExposeNamedActions": len(actor_results) == 10 and all(item["actionCount"] >= 9 for item in actor_results),
        "exactRuntimeActionContractValidated": all(
            set(HERO_RUNTIME_ACTIONS if item["id"] in HERO_CLIPS else NONHERO_RUNTIME_ACTIONS)
            <= set(item["namedActions"])
            for item in actor_results
        ),
        "bothHeroesHaveExpandedLocomotionAndThreeHitBasicVariants": all(
            item["locomotionExpanded"] and item["comboExpanded"]
            for item in actor_results if item["id"] in HERO_CLIPS
        ),
        "everyHeroCombatSlotHasCharacterActionAnd3DModel": len(skill_results) == 10,
        "allSkillAssetsAreReal3DGeometry": all(not item["flatTexturePlane"] for item in skill_results),
        "qaRenderCount": sum(bool(item.get("qaRender")) for item in skill_results),
        "structuralValidation": "pass",
        "runtimeIntegration": "pending C++ opt-in; this report covers the requested asset side only",
        "manifests": [rel(ANIMATION_MANIFEST), rel(SKILL_MANIFEST), rel(STEP7_MANIFEST)],
        "models": [{"id": item["id"], "path": item["model"], "sha256": item["glbAudit"]["sha256"]} for item in skill_results],
    }
    write_json(ACCEPTANCE_REPORT, report)
    return report


def main():
    if bpy.app.version < BLENDER_MINIMUM:
        raise RuntimeError(f"Blender 4.5+ required; found {bpy.app.version_string}")
    args = parse_args()
    if args.validate_only:
        actors, skills = existing_results_for_validation()
        for result in actors:
            validate_actor(result)
        for result in skills:
            validate_skill(result)
        write_manifests(actors, skills, no_render=not any(item.get("qaRender") for item in skills))
        print(f"STEP7_VALIDATION=PASS actors={len(actors)} skills={len(skills)}", flush=True)
        return

    actor_results = []
    skill_results = []
    if not args.skip_actors:
        actor_ids = list(ACTORS) if args.actor == "all" else [args.actor]
        for actor_id in actor_ids:
            print(f"STEP7_BUILD_ACTOR={actor_id}", flush=True)
            actor_results.append(build_actor(actor_id))
            validate_actor(actor_results[-1])
        if args.actor != "all" and os.path.isfile(STEP7_MANIFEST):
            existing = json.load(open(STEP7_MANIFEST, encoding="utf-8")).get("actors", [])
            merged = {item["id"]: item for item in existing}
            merged.update({item["id"]: item for item in actor_results})
            actor_results = [merged[actor_id] for actor_id in ACTORS if actor_id in merged]
    elif os.path.isfile(STEP7_MANIFEST):
        actor_results = json.load(open(STEP7_MANIFEST, encoding="utf-8")).get("actors", [])

    if not args.skip_skills:
        for definition in SKILLS:
            print(f"STEP7_BUILD_SKILL={definition['id']}", flush=True)
            skill_results.append(build_skill(dict(definition), no_render=args.no_render))
            validate_skill(skill_results[-1])
    elif os.path.isfile(STEP7_MANIFEST):
        skill_results = json.load(open(STEP7_MANIFEST, encoding="utf-8")).get("skills", [])

    # A full acceptance report is only truthful when all 10 actors and all 10
    # skills have been built.  Partial runs remain useful for development but
    # intentionally do not stamp completion.
    complete = len(actor_results) == len(ACTORS) and len(skill_results) == len(SKILLS)
    if complete:
        report = write_manifests(actor_results, skill_results, args.no_render)
        print("STEP7_ACCEPTANCE=" + json.dumps(report), flush=True)
    else:
        partial = {
            "schemaVersion": 1,
            "pipeline": PIPELINE_ID,
            "status": "partial_development_build",
            "actors": actor_results,
            "skills": skill_results,
        }
        write_json(STEP7_MANIFEST, partial)
        print(f"STEP7_PARTIAL actors={len(actor_results)} skills={len(skill_results)}", flush=True)


if __name__ == "__main__":
    main()
