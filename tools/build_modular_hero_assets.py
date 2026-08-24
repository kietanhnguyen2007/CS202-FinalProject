"""Build clean, textured modular hero meshes from the production_v3 Hunyuan GLBs.

Run with Blender 4.5 LTS (all assets are built when --asset is omitted):

    blender --background --factory-startup \
      --python tools/build_modular_hero_assets.py -- \
      --asset knight_body --asset knight_greatsword

The generated .blend/.glb files are intentionally unrigged.  They contain one
mesh with applied transforms, +Z up, one material and one UV layer named
``UVMap``.  Bodies are grounded at z=0; weapons instead place the authored grip
at local origin.  Source-view projection UVs/materials only exist while baking
and are removed before export.
"""

from __future__ import annotations

import argparse
from array import array
import bmesh
import bpy
from collections import deque
import colorsys
import json
import math
import numpy as np
import os
import sys
import time
from mathutils import Vector
from mathutils.bvhtree import BVHTree


ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
SURVIVAL3D = os.path.join(ROOT, "assets", "survival3d")
PRODUCTION = os.path.join(SURVIVAL3D, "production_v3")
VIEW_ROOT = os.path.join(SURVIVAL3D, "concepts", "characters", "modular", "views")
QA_ROOT = os.path.join(PRODUCTION, "qa", "clean")

ASSETS = {
    "knight_body": {
        "character": "knight",
        "object_name": "KnightBody",
        "kind": "body",
        "height": 2.0,
        "vertex_limit": 18_900,
        "face_limit": 38_000,
        "palette": "purple_graphite",
        "raw": "raw/heroes/knight/knight_body_hunyuan_raw.glb",
    },
    "knight_greatsword": {
        "character": "knight",
        "object_name": "KnightGreatsword",
        "kind": "weapon",
        "height": 1.55,
        "vertex_limit": 2_950,
        "face_limit": 6_000,
        "grip_fraction": 0.895,
        "grip_band": (0.84, 0.95),
        "raw": "raw/heroes/knight/knight_greatsword_hunyuan_raw.glb",
    },
    "magic_caster_body": {
        "character": "magic_caster",
        "object_name": "MagicCasterBody",
        "kind": "body",
        "height": 2.0,
        "vertex_limit": 18_900,
        "face_limit": 38_000,
        "palette": "blue_white_navy",
        "raw": "raw/heroes/magic_caster/magic_caster_body_hunyuan_raw.glb",
    },
    "magic_caster_staff": {
        "character": "magic_caster",
        "object_name": "MagicCasterStaff",
        "kind": "weapon",
        "height": 1.65,
        "vertex_limit": 2_450,
        "face_limit": 5_000,
        "grip_fraction": 0.467,
        "grip_band": (0.42, 0.52),
        "raw": "raw/heroes/magic_caster/magic_caster_staff_hunyuan_raw.glb",
    },
}


def log(message: str) -> None:
    print(f"[modular-assets] {message}", flush=True)


def arguments() -> argparse.Namespace:
    values = sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else []
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--asset",
        dest="assets",
        action="append",
        choices=tuple(ASSETS),
        help="Asset to build; repeat for several. Defaults to all four.",
    )
    parser.add_argument("--atlas-size", type=int, default=2048)
    parser.add_argument(
        "--no-three-quarter",
        action="store_true",
        help="Only render the required front/side/back QA views.",
    )
    result = parser.parse_args(values)
    if result.atlas_size < 512 or result.atlas_size > 8192:
        parser.error("--atlas-size must be between 512 and 8192")
    if result.atlas_size & (result.atlas_size - 1):
        parser.error("--atlas-size must be a power of two")
    result.assets = result.assets or list(ASSETS)
    return result


def absolute_production(relative_path: str) -> str:
    return os.path.normpath(os.path.join(PRODUCTION, relative_path))


def output_paths(asset_name: str, spec: dict) -> dict[str, str]:
    directory = os.path.join(PRODUCTION, "heroes", spec["character"])
    os.makedirs(directory, exist_ok=True)
    return {
        "directory": directory,
        "blend": os.path.join(directory, f"{asset_name}_production_v3.blend"),
        "glb": os.path.join(directory, f"{asset_name}_production_v3.glb"),
        "texture": os.path.join(directory, f"{asset_name}_basecolor.png"),
    }


def select_only(objects) -> None:
    if bpy.context.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.hide_set(False)
        obj.hide_viewport = False
        obj.hide_render = False
        obj.select_set(True)
    if objects:
        bpy.context.view_layer.objects.active = objects[0]


def mesh_bounds(mesh_object) -> tuple[float, float, float, float, float, float]:
    if not mesh_object.data.vertices:
        raise RuntimeError(f"{mesh_object.name} has no vertices")
    points = (mesh_object.matrix_world @ vertex.co for vertex in mesh_object.data.vertices)
    first = next(points)
    xmin = xmax = first.x
    ymin = ymax = first.y
    zmin = zmax = first.z
    for point in points:
        xmin, xmax = min(xmin, point.x), max(xmax, point.x)
        ymin, ymax = min(ymin, point.y), max(ymax, point.y)
        zmin, zmax = min(zmin, point.z), max(zmax, point.z)
    return xmin, xmax, ymin, ymax, zmin, zmax


def connected_components(mesh_data) -> list[list[int]]:
    """Return vertex-index components, largest first, without recursion."""
    vertex_count = len(mesh_data.vertices)
    adjacency = [[] for _ in range(vertex_count)]
    for edge in mesh_data.edges:
        left, right = edge.vertices
        adjacency[left].append(right)
        adjacency[right].append(left)
    unseen = bytearray(b"\x01") * vertex_count
    components = []
    for seed in range(vertex_count):
        if not unseen[seed]:
            continue
        unseen[seed] = 0
        stack = [seed]
        component = []
        while stack:
            current = stack.pop()
            component.append(current)
            for neighbor in adjacency[current]:
                if unseen[neighbor]:
                    unseen[neighbor] = 0
                    stack.append(neighbor)
        components.append(component)
    return sorted(components, key=len, reverse=True)


def remove_tiny_components(mesh_object) -> dict:
    """Remove every shell strictly smaller than 0.1% of source vertices."""
    mesh_data = mesh_object.data
    total = len(mesh_data.vertices)
    components = connected_components(mesh_data)
    threshold = total * 0.001
    discarded_components = [component for component in components if len(component) < threshold]
    discarded_vertices = sum(len(component) for component in discarded_components)
    if discarded_vertices:
        indices = {index for component in discarded_components for index in component}
        bm = bmesh.new()
        bm.from_mesh(mesh_data)
        bm.verts.ensure_lookup_table()
        bmesh.ops.delete(
            bm,
            geom=[bm.verts[index] for index in sorted(indices)],
            context="VERTS",
        )
        bm.to_mesh(mesh_data)
        bm.free()
        mesh_data.validate(clean_customdata=False)
        mesh_data.update()
    kept_sizes = [len(component) for component in components if len(component) >= threshold]
    log(
        f"components: {len(components)} input; removed {len(discarded_components)} "
        f"shell(s)/{discarded_vertices} vertices below {threshold:.3f}; "
        f"kept sizes {kept_sizes[:12]}"
    )
    return {
        "input_components": len(components),
        "threshold_vertices": threshold,
        "removed_components": len(discarded_components),
        "removed_vertices": discarded_vertices,
        "kept_component_sizes_before_decimate": kept_sizes,
    }


def import_joined_mesh(asset_name: str, spec: dict):
    raw_path = absolute_production(spec["raw"])
    if not os.path.isfile(raw_path):
        raise RuntimeError(f"Missing raw GLB: {raw_path}")
    bpy.ops.import_scene.gltf(filepath=raw_path)
    meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    if not meshes:
        raise RuntimeError(f"No mesh imported from {raw_path}")
    select_only(meshes)
    if len(meshes) > 1:
        bpy.context.view_layer.objects.active = meshes[0]
        bpy.ops.object.join()
    mesh_object = bpy.context.object
    world = mesh_object.matrix_world.copy()
    mesh_object.parent = None
    mesh_object.matrix_world = world
    select_only([mesh_object])
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    for obj in list(bpy.context.scene.objects):
        if obj is not mesh_object:
            bpy.data.objects.remove(obj, do_unlink=True)
    mesh_object.name = spec["object_name"]
    mesh_object.data.name = spec["object_name"] + "_Mesh"
    mesh_object.data.materials.clear()
    log(
        f"imported {asset_name}: joined {len(meshes)} mesh object(s), "
        f"{len(mesh_object.data.vertices)} vertices/{len(mesh_object.data.polygons)} faces"
    )
    return mesh_object, raw_path


def normalize_mesh(mesh_object, target_height: float) -> tuple[float, float, float]:
    xmin, xmax, ymin, ymax, zmin, zmax = mesh_bounds(mesh_object)
    source_height = zmax - zmin
    if source_height <= 1e-8:
        raise RuntimeError(f"Cannot normalize zero-height mesh {mesh_object.name}")
    scale = target_height / source_height
    center_x = (xmin + xmax) * 0.5
    center_y = (ymin + ymax) * 0.5
    for vertex in mesh_object.data.vertices:
        vertex.co.x = (vertex.co.x - center_x) * scale
        vertex.co.y = (vertex.co.y - center_y) * scale
        vertex.co.z = (vertex.co.z - zmin) * scale
    mesh_object.data.update()
    mesh_object.location = (0.0, 0.0, 0.0)
    mesh_object.rotation_euler = (0.0, 0.0, 0.0)
    mesh_object.scale = (1.0, 1.0, 1.0)
    xmin, xmax, ymin, ymax, zmin, zmax = mesh_bounds(mesh_object)
    dimensions = (xmax - xmin, ymax - ymin, zmax - zmin)
    if abs(zmin) > 1e-5 or abs(dimensions[2] - target_height) > 1e-4:
        raise RuntimeError(
            f"Normalization failed for {mesh_object.name}: zmin={zmin}, height={dimensions[2]}"
        )
    log(
        f"normalized {mesh_object.name}: bounds "
        f"{dimensions[0]:.5f} x {dimensions[1]:.5f} x {dimensions[2]:.5f} m"
    )
    return dimensions


def _median_coordinate(values: list[float]) -> float:
    if not values:
        raise RuntimeError("Cannot take median of an empty coordinate list")
    values = sorted(values)
    middle = len(values) // 2
    if len(values) % 2:
        return values[middle]
    return (values[middle - 1] + values[middle]) * 0.5


def place_weapon_grip_at_origin(mesh_object, spec: dict) -> dict:
    """Move weapon mesh-local vertices so the runtime attachment grip is zero."""
    if spec["kind"] != "weapon":
        return {"grip_origin_error_m": 0.0, "grip_vertex_count": 0}
    xmin, xmax, ymin, ymax, zmin, zmax = mesh_bounds(mesh_object)
    height = zmax - zmin
    band_min, band_max = spec["grip_band"]
    handle = [
        vertex.co
        for vertex in mesh_object.data.vertices
        if zmin + height * band_min <= vertex.co.z <= zmin + height * band_max
    ]
    if len(handle) < 8:
        raise RuntimeError(
            f"Could not find a stable handle cross-section for {mesh_object.name}: {len(handle)} vertices"
        )
    grip = Vector(
        (
            _median_coordinate([point.x for point in handle]),
            _median_coordinate([point.y for point in handle]),
            zmin + height * spec["grip_fraction"],
        )
    )
    for vertex in mesh_object.data.vertices:
        vertex.co -= grip
    mesh_object.data.update()
    # Recalculate from the shifted handle region for an explicit <=1 cm audit.
    new_xmin, new_xmax, new_ymin, new_ymax, new_zmin, new_zmax = mesh_bounds(mesh_object)
    shifted_handle = [
        vertex.co
        for vertex in mesh_object.data.vertices
        if new_zmin + height * band_min <= vertex.co.z <= new_zmin + height * band_max
    ]
    measured = Vector(
        (
            _median_coordinate([point.x for point in shifted_handle]),
            _median_coordinate([point.y for point in shifted_handle]),
            new_zmin + height * spec["grip_fraction"],
        )
    )
    grip_error = measured.length
    if grip_error > 0.01:
        raise RuntimeError(f"Grip-origin error for {mesh_object.name}: {grip_error:.6f} m")
    mesh_object["grip_local"] = (0.0, 0.0, 0.0)
    mesh_object["grip_fraction_from_lowest_point"] = spec["grip_fraction"]
    log(
        f"placed weapon grip at local origin using {len(handle)} handle vertices; "
        f"source grip=({grip.x:.5f},{grip.y:.5f},{grip.z:.5f}), error={grip_error:.6f} m"
    )
    return {
        "grip_source_local_m": [round(value, 6) for value in grip],
        "grip_origin_error_m": round(grip_error, 8),
        "grip_vertex_count": len(handle),
    }


def decimate_to_game_limits(mesh_object, vertex_limit: int, face_limit: int) -> tuple[int, int]:
    before = len(mesh_object.data.vertices)
    current = before
    current_faces = len(mesh_object.data.polygons)
    attempt = 0
    while (current > vertex_limit or current_faces > face_limit) and attempt < 6:
        attempt += 1
        # Ratio is face based, so leave a small safety margin for meshes whose
        # boundary/non-manifold vertex ratio differs from their interior.
        ratio = min(
            0.999,
            vertex_limit / current,
            face_limit / current_faces,
        ) * (0.985 if attempt == 1 else 0.97)
        modifier = mesh_object.modifiers.new(f"SilhouetteDecimate_{attempt}", "DECIMATE")
        modifier.decimate_type = "COLLAPSE"
        modifier.ratio = max(0.001, ratio)
        modifier.use_collapse_triangulate = True
        select_only([mesh_object])
        bpy.ops.object.modifier_apply(modifier=modifier.name)
        mesh_object.data.validate(clean_customdata=False)
        mesh_object.data.update()
        next_count = len(mesh_object.data.vertices)
        next_faces = len(mesh_object.data.polygons)
        if next_count >= current and next_faces >= current_faces:
            break
        current, current_faces = next_count, next_faces
    if current > vertex_limit or current_faces > face_limit:
        raise RuntimeError(
            f"Decimation could not reach game limits for {mesh_object.name}: "
            f"{current}/{vertex_limit} vertices, {current_faces}/{face_limit} faces"
        )
    for polygon in mesh_object.data.polygons:
        polygon.use_smooth = True
    mesh_object.data.update()
    log(
        f"silhouette decimation {mesh_object.name}: {before} -> {current} vertices, "
        f"{current_faces} faces (limits {vertex_limit} vertices/{face_limit} faces)"
    )
    return before, current


def create_clean_uv_map(mesh_object) -> None:
    mesh_data = mesh_object.data
    while mesh_data.uv_layers:
        mesh_data.uv_layers.remove(mesh_data.uv_layers[0])
    mesh_data.uv_layers.new(name="UVMap")
    mesh_data.uv_layers.active_index = 0
    select_only([mesh_object])
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(
        angle_limit=math.radians(62.0),
        margin_method="SCALED",
        rotate_method="AXIS_ALIGNED",
        island_margin=0.008,
        area_weight=0.15,
        correct_aspect=True,
        scale_to_bounds=False,
    )
    bpy.ops.object.mode_set(mode="OBJECT")
    uv_layer = mesh_data.uv_layers.get("UVMap")
    if uv_layer is None or len(uv_layer.data) != len(mesh_data.loops):
        raise RuntimeError(f"UV unwrap failed for {mesh_object.name}")
    minimum = [float("inf"), float("inf")]
    maximum = [float("-inf"), float("-inf")]
    for item in uv_layer.data:
        for axis in (0, 1):
            value = item.uv[axis]
            if not math.isfinite(value):
                raise RuntimeError(f"Non-finite UV on {mesh_object.name}")
            minimum[axis] = min(minimum[axis], value)
            maximum[axis] = max(maximum[axis], value)
    if minimum[0] < -1e-4 or minimum[1] < -1e-4 or maximum[0] > 1.0001 or maximum[1] > 1.0001:
        raise RuntimeError(
            f"UVMap outside 0..1 for {mesh_object.name}: min={minimum}, max={maximum}"
        )
    log(
        f"created UVMap: {len(uv_layer.data)} loop UVs, "
        f"range ({minimum[0]:.4f},{minimum[1]:.4f})-({maximum[0]:.4f},{maximum[1]:.4f})"
    )


def _dilated_mask(mask: bytearray, width: int, height: int, radius: int = 1) -> bytearray:
    result = bytearray(mask)
    for y in range(height):
        for x in range(width):
            index = y * width + x
            if not mask[index]:
                continue
            for oy in range(-radius, radius + 1):
                ny = y + oy
                if ny < 0 or ny >= height:
                    continue
                start = ny * width
                for ox in range(-radius, radius + 1):
                    nx = x + ox
                    if 0 <= nx < width:
                        result[start + nx] = 1
    return result


def _median(values: list[float]) -> float:
    values.sort()
    middle = len(values) // 2
    if len(values) % 2:
        return values[middle]
    return (values[middle - 1] + values[middle]) * 0.5


def foreground_mask_from_row_background(image):
    """Separate the subject from the nearly grayscale background.

    Chroma alone misses black cloth, white trim and metallic highlights.  Each
    row therefore gets its own background estimate from the two edge bands and
    a pixel is foreground when either chroma or RGB distance from that row's
    grayscale background is significant.
    """
    width, height = image.size
    pixels = array("f", [0.0]) * (width * height * 4)
    image.pixels.foreach_get(pixels)
    edge_band = max(12, width // 20)
    row_backgrounds = []
    for y in range(height):
        candidates = []
        edge_xs = list(range(edge_band)) + list(range(width - edge_band, width))
        for x in edge_xs:
            base = (y * width + x) * 4
            red, green, blue = pixels[base], pixels[base + 1], pixels[base + 2]
            if max(red, green, blue) - min(red, green, blue) < 0.028:
                candidates.append((red, green, blue))
        if len(candidates) < edge_band // 2:
            candidates = []
            for x in edge_xs:
                base = (y * width + x) * 4
                candidates.append((pixels[base], pixels[base + 1], pixels[base + 2]))
        row_backgrounds.append(
            (
                _median([value[0] for value in candidates]),
                _median([value[1] for value in candidates]),
                _median([value[2] for value in candidates]),
            )
        )
    foreground = bytearray(width * height)
    chroma_threshold = 0.030
    # The vignetted studio background varies perceptibly across X even within a
    # row.  A conservative distance gate keeps that low-frequency gray field
    # out, while black cloth and white/silver trim remain far beyond the gate.
    distance_squared_threshold = 0.160 * 0.160
    for y in range(height):
        bg_red, bg_green, bg_blue = row_backgrounds[y]
        for x in range(width):
            pixel_index = y * width + x
            base = pixel_index * 4
            red, green, blue, alpha = (
                pixels[base],
                pixels[base + 1],
                pixels[base + 2],
                pixels[base + 3],
            )
            chroma = max(red, green, blue) - min(red, green, blue)
            distance_squared = (
                (red - bg_red) ** 2 + (green - bg_green) ** 2 + (blue - bg_blue) ** 2
            )
            if alpha > 0.05 and (
                chroma > chroma_threshold or distance_squared > distance_squared_threshold
            ):
                foreground[pixel_index] = 1
    return pixels, foreground


def detect_subject_bounds(
    image,
    full_mask: bytearray,
) -> tuple[tuple[float, float, float, float], set[int], int, int]:
    """Find the main foreground cluster in an opaque gray reference.

    The supplied view PNGs are crops of three-view sheets.  Some subjects are
    intentionally off-centre and a side crop can contain a small fragment of a
    neighbouring view, so full-image UV projection would misalign the face and
    armor.  A per-row background-difference mask includes achromatic face/armor
    details which a chroma-only detector would discard.  A downsampled, dilated
    connected-component mask isolates the subject without hard-coded crops.
    Returned coordinates use Blender's bottom-left UV convention.
    """
    width, height = image.size
    step = max(2, min(width, height) // 128)
    grid_width = (width + step - 1) // step
    grid_height = (height + step - 1) // step
    core = bytearray(grid_width * grid_height)
    for pixel_index, is_foreground in enumerate(full_mask):
        if is_foreground:
            x = pixel_index % width
            y = pixel_index // width
            core[(y // step) * grid_width + (x // step)] = 1
    expanded = _dilated_mask(core, grid_width, grid_height, radius=2)
    unseen = bytearray(expanded)
    components = []
    for seed in range(len(unseen)):
        if not unseen[seed]:
            continue
        unseen[seed] = 0
        stack = [seed]
        cells = []
        while stack:
            current = stack.pop()
            cells.append(current)
            x = current % grid_width
            y = current // grid_width
            for oy in (-1, 0, 1):
                ny = y + oy
                if ny < 0 or ny >= grid_height:
                    continue
                row = ny * grid_width
                for ox in (-1, 0, 1):
                    nx = x + ox
                    if nx < 0 or nx >= grid_width:
                        continue
                    neighbor = row + nx
                    if unseen[neighbor]:
                        unseen[neighbor] = 0
                        stack.append(neighbor)
        core_cells = [cell for cell in cells if core[cell]]
        if core_cells:
            components.append(core_cells)
    if not components:
        raise RuntimeError(f"Could not detect foreground subject in {image.filepath}")
    # Vertical reach is useful for long thin weapons; count dominates bodies.
    def component_score(cells):
        ys = [cell // grid_width for cell in cells]
        return len(cells) + (max(ys) - min(ys) + 1) * 2

    primary = max(components, key=component_score)
    subject = list(primary)
    # Dark cloth can split saturated armor, hair and boots into separate color
    # islands even though they are one subject.  Merge vertically adjacent or
    # horizontally overlapping islands into the main cluster.  A fragment from
    # a neighbouring turnaround panel remains excluded because it is separated
    # horizontally by a wide gray strip.
    remaining = [component for component in components if component is not primary]

    def cell_bounds(cells):
        cell_xs = [cell % grid_width for cell in cells]
        cell_ys = [cell // grid_width for cell in cells]
        return min(cell_xs), max(cell_xs), min(cell_ys), max(cell_ys)

    changed = True
    while changed:
        changed = False
        main_xmin, main_xmax, main_ymin, main_ymax = cell_bounds(subject)
        for candidate in list(remaining):
            cand_xmin, cand_xmax, cand_ymin, cand_ymax = cell_bounds(candidate)
            x_overlap = min(main_xmax, cand_xmax) - max(main_xmin, cand_xmin) + 1
            y_overlap = min(main_ymax, cand_ymax) - max(main_ymin, cand_ymin) + 1
            x_gap = max(0, max(main_xmin, cand_xmin) - min(main_xmax, cand_xmax) - 1)
            y_gap = max(0, max(main_ymin, cand_ymin) - min(main_ymax, cand_ymax) - 1)
            aligned_vertically = x_overlap > 0 and y_gap <= max(3, int(grid_height * 0.14))
            adjacent_side_part = y_overlap > 0 and x_gap <= max(3, int(grid_width * 0.075))
            if aligned_vertically or adjacent_side_part:
                subject.extend(candidate)
                remaining.remove(candidate)
                changed = True
    xs = [cell % grid_width for cell in subject]
    ys = [cell // grid_width for cell in subject]
    xmin = max(0, min(xs) * step - step)
    xmax = min(width - 1, (max(xs) + 1) * step + step)
    ymin = max(0, min(ys) * step - step)
    ymax = min(height - 1, (max(ys) + 1) * step + step)
    if xmax - xmin < width * 0.015 or ymax - ymin < height * 0.08:
        raise RuntimeError(f"Implausible foreground bounds for {image.filepath}: {(xmin,xmax,ymin,ymax)}")
    bounds = (
        (xmin + 0.5) / width,
        (xmax + 0.5) / width,
        (ymin + 0.5) / height,
        (ymax + 0.5) / height,
    )
    log(
        f"projection crop {os.path.basename(image.filepath)}: "
        f"u={bounds[0]:.4f}..{bounds[1]:.4f}, v={bounds[2]:.4f}..{bounds[3]:.4f}"
    )
    return bounds, set(subject), step, grid_width


def nearest_fill_projection_image(
    original,
    pixels: array,
    foreground: bytearray,
    subject_cells: set[int],
    step: int,
    grid_width: int,
    prepared_name: str,
):
    """Replace every background texel by its nearest subject texel.

    Projected geometry never matches the 2D silhouette perfectly.  Leaving the
    original studio background in place therefore creates gray tears on the 3D
    mesh.  A multi-source Manhattan flood fills the whole temporary source with
    nearest authored subject color before projection; no generic tint is added.
    """
    width, height = original.size
    pixel_count = width * height
    owners = array("i", [-1]) * pixel_count
    queue = deque()
    # Remove the antialiased one/two-pixel silhouette fringe before flooding.
    # Otherwise a gray/source blend at the outline becomes a large gray stripe
    # after nearest-fill, defeating the purpose of background removal.
    safe_foreground = bytearray(foreground)
    for _pass in range(2):
        eroded = bytearray(pixel_count)
        for pixel_index, is_foreground in enumerate(safe_foreground):
            if not is_foreground:
                continue
            x = pixel_index % width
            if (
                x > 0
                and x + 1 < width
                and pixel_index >= width
                and pixel_index + width < pixel_count
                and safe_foreground[pixel_index - 1]
                and safe_foreground[pixel_index + 1]
                and safe_foreground[pixel_index - width]
                and safe_foreground[pixel_index + width]
            ):
                eroded[pixel_index] = 1
        safe_foreground = eroded
    for pixel_index, is_foreground in enumerate(safe_foreground):
        if not is_foreground:
            continue
        base = pixel_index * 4
        red, green, blue = pixels[base], pixels[base + 1], pixels[base + 2]
        chroma = max(red, green, blue) - min(red, green, blue)
        luminance = red * 0.2126 + green * 0.7152 + blue * 0.0722
        # The floor/shadow and vignette are neutral middle gray.  They can be
        # locally far from an edge-derived row estimate, so reject that narrow
        # background family explicitly.  Authored black cloth stays below the
        # luminance band; white trim stays above it; colored armor/hair passes
        # the chroma gate.
        if chroma < 0.025 and 0.025 < luminance < 0.50:
            continue
        x = pixel_index % width
        y = pixel_index // width
        cell = (y // step) * grid_width + (x // step)
        if cell not in subject_cells:
            continue
        owners[pixel_index] = pixel_index
        queue.append(pixel_index)
    if not queue:
        raise RuntimeError(f"No subject pixels available to fill {original.filepath}")
    while queue:
        current = queue.popleft()
        owner = owners[current]
        x = current % width
        if x > 0 and owners[current - 1] < 0:
            owners[current - 1] = owner
            queue.append(current - 1)
        if x + 1 < width and owners[current + 1] < 0:
            owners[current + 1] = owner
            queue.append(current + 1)
        if current >= width and owners[current - width] < 0:
            owners[current - width] = owner
            queue.append(current - width)
        if current + width < pixel_count and owners[current + width] < 0:
            owners[current + width] = owner
            queue.append(current + width)
    filled = array("f", [0.0]) * (pixel_count * 4)
    for pixel_index, owner in enumerate(owners):
        source = owner * 4
        target = pixel_index * 4
        filled[target] = pixels[source]
        filled[target + 1] = pixels[source + 1]
        filled[target + 2] = pixels[source + 2]
        filled[target + 3] = 1.0
    # A single nearest owner is safe chromatically, but its Manhattan Voronoi
    # cells become conspicuous horizontal/vertical bands when a 3D silhouette
    # projects a few pixels outside its 2D reference.  Diffuse only those
    # extrapolated texels with wide separable box filters, while restoring every
    # authored seed after each pass.  The result remains entirely derived from
    # the supplied view, yet changes smoothly beyond the photographed outline.
    filled_np = np.asarray(filled, dtype=np.float32).reshape(height, width, 4).copy()
    owner_np = np.asarray(owners, dtype=np.int32).reshape(height, width)
    seed_mask = owner_np.reshape(-1) == np.arange(pixel_count, dtype=np.int32)
    seed_mask = seed_mask.reshape(height, width)
    authored_rgb = filled_np[:, :, :3].copy()

    def box_blur_rgb(values, radius):
        window = radius * 2 + 1
        padded_x = np.pad(values, ((0, 0), (radius, radius), (0, 0)), mode="edge")
        cumulative_x = np.concatenate(
            (np.zeros((height, 1, 3), dtype=np.float32), np.cumsum(padded_x, axis=1)),
            axis=1,
        )
        horizontal = (cumulative_x[:, window : window + width] - cumulative_x[:, :width]) / window
        padded_y = np.pad(horizontal, ((radius, radius), (0, 0), (0, 0)), mode="edge")
        cumulative_y = np.concatenate(
            (np.zeros((1, width, 3), dtype=np.float32), np.cumsum(padded_y, axis=0)),
            axis=0,
        )
        return (cumulative_y[window : window + height] - cumulative_y[:height]) / window

    smoothed = filled_np[:, :, :3]
    for _pass in range(3):
        smoothed = box_blur_rgb(smoothed, radius=max(8, min(width, height) // 24))
        smoothed[seed_mask] = authored_rgb[seed_mask]
    filled_np[:, :, :3] = smoothed
    filled = array("f", filled_np.reshape(-1))
    prepared = bpy.data.images.new(
        prepared_name,
        width=width,
        height=height,
        alpha=False,
        float_buffer=False,
    )
    prepared.colorspace_settings.name = "sRGB"
    prepared.pixels.foreach_set(filled)
    prepared.update()
    return prepared


def prepare_reference_view(original, prepared_name: str):
    pixels, foreground = foreground_mask_from_row_background(original)
    bounds, subject_cells, step, grid_width = detect_subject_bounds(original, foreground)
    prepared = nearest_fill_projection_image(
        original,
        pixels,
        foreground,
        subject_cells,
        step,
        grid_width,
        prepared_name,
    )
    return prepared, bounds


def load_reference_views(asset_name: str):
    images = {}
    bounds = {}
    for view in ("front", "back", "right"):
        path = os.path.join(VIEW_ROOT, f"{asset_name}_{view}.png")
        if not os.path.isfile(path):
            raise RuntimeError(f"Missing projection source: {path}")
        original = bpy.data.images.load(path, check_existing=False)
        original.name = f"{asset_name}_{view}_OriginalReference"
        original.colorspace_settings.name = "sRGB"
        prepared, bounds[view] = prepare_reference_view(
            original,
            f"{asset_name}_{view}_ProjectionSource_Inpainted",
        )
        images[view] = prepared
        bpy.data.images.remove(original)
    return images, bounds


def create_palette_safe_image(source_image, palette: str, name: str):
    """Nearest-fill from authored palette pixels, excluding skin/eye/background hues.

    This is used outside semantic face/hand regions.  Every retained texel is an
    actual reference-image color: Knight uses purple/graphite candidates; Mage
    uses blue/navy/white candidates.  It prevents a profile face from appearing
    on the ponytail and suppresses pale gray projection blends on armor.
    """
    width, height = source_image.size
    pixel_count = width * height
    pixels = array("f", [0.0]) * (pixel_count * 4)
    source_image.pixels.foreach_get(pixels)
    owners = array("i", [-1]) * pixel_count
    queue = deque()
    for pixel_index in range(pixel_count):
        base = pixel_index * 4
        red, green, blue = pixels[base], pixels[base + 1], pixels[base + 2]
        high = max(red, green, blue)
        low = min(red, green, blue)
        chroma = high - low
        if palette == "purple_graphite":
            keep = (
                high < 0.070
                or (
                    chroma > 0.024
                    and blue > green * 1.08
                    and red > green * 1.02
                )
            )
        elif palette == "blue_white_navy":
            keep = (
                high < 0.070
                or (chroma > 0.026 and blue > red * 1.18 and blue > green * 1.03)
                or (low > 0.58 and chroma < 0.055)
            )
        else:
            raise RuntimeError(f"Unknown safe palette {palette}")
        if keep:
            owners[pixel_index] = pixel_index
            queue.append(pixel_index)
    if not queue:
        raise RuntimeError(f"No candidate pixels for palette {palette} in {source_image.name}")
    while queue:
        current = queue.popleft()
        owner = owners[current]
        x = current % width
        if x > 0 and owners[current - 1] < 0:
            owners[current - 1] = owner
            queue.append(current - 1)
        if x + 1 < width and owners[current + 1] < 0:
            owners[current + 1] = owner
            queue.append(current + 1)
        if current >= width and owners[current - width] < 0:
            owners[current - width] = owner
            queue.append(current - width)
        if current + width < pixel_count and owners[current + width] < 0:
            owners[current + width] = owner
            queue.append(current + width)
    safe_pixels = array("f", [0.0]) * (pixel_count * 4)
    for pixel_index, owner in enumerate(owners):
        source = owner * 4
        target = pixel_index * 4
        red, green, blue = pixels[source], pixels[source + 1], pixels[source + 2]
        if palette == "purple_graphite":
            hue, saturation, value = colorsys.rgb_to_hsv(red, green, blue)
            red, green, blue = colorsys.hsv_to_rgb(
                hue,
                min(1.0, saturation * 1.20),
                value * 0.80,
            )
        elif palette == "blue_white_navy":
            hue, saturation, value = colorsys.rgb_to_hsv(red, green, blue)
            if saturation > 0.10:
                saturation = min(1.0, saturation * 1.55)
                value *= 0.627
            else:
                value *= 0.93
            red, green, blue = colorsys.hsv_to_rgb(hue, saturation, value)
        safe_pixels[target] = red
        safe_pixels[target + 1] = green
        safe_pixels[target + 2] = blue
        safe_pixels[target + 3] = 1.0
    result = bpy.data.images.new(name, width=width, height=height, alpha=False, float_buffer=False)
    result.colorspace_settings.name = "sRGB"
    result.pixels.foreach_set(safe_pixels)
    result.update()
    return result


def create_palette_safe_views(asset_name: str, spec: dict, source_images: dict):
    if spec["kind"] != "body":
        return None
    results = {
        view: create_palette_safe_image(
            source_images[view],
            spec["palette"],
            f"{asset_name}_{view}_{spec['palette']}_SemanticSafe",
        )
        for view in ("front", "back", "right")
    }
    if spec.get("palette") == "purple_graphite":
        # Keep the three authored Knight palettes on the same exposure without
        # flattening their local detail.  Only a small value gain is allowed.
        buffers = {}
        mean_values = {}
        for view, image in results.items():
            pixels = array("f", [0.0]) * (image.size[0] * image.size[1] * 4)
            image.pixels.foreach_get(pixels)
            buffers[view] = pixels
            mean_values[view] = sum(
                max(pixels[index], pixels[index + 1], pixels[index + 2])
                for index in range(0, len(pixels), 4)
            ) / (len(pixels) // 4)
        reference = mean_values["front"]
        for view in ("back", "right"):
            gain = max(0.92, min(1.08, reference / max(1e-8, mean_values[view])))
            pixels = buffers[view]
            for index in range(0, len(pixels), 4):
                pixels[index] = min(1.0, pixels[index] * gain)
                pixels[index + 1] = min(1.0, pixels[index + 1] * gain)
                pixels[index + 2] = min(1.0, pixels[index + 2] * gain)
            results[view].pixels.foreach_set(pixels)
            results[view].update()
            log(f"normalized {asset_name} {view} palette value gain={gain:.4f}")
    return results


def create_selective_blue_grade_views(asset_name: str, spec: dict, source_images: dict):
    """Grade Mage blue classes while preserving authored skin, eyes and trim."""
    if spec.get("palette") != "blue_white_navy":
        return source_images
    results = {}
    for view, source_image in source_images.items():
        width, height = source_image.size
        pixels = array("f", [0.0]) * (width * height * 4)
        source_image.pixels.foreach_get(pixels)
        for pixel_index in range(width * height):
            base = pixel_index * 4
            red, green, blue = pixels[base], pixels[base + 1], pixels[base + 2]
            chroma = max(red, green, blue) - min(red, green, blue)
            if chroma > 0.025 and blue > red * 1.15 and blue > green * 1.02:
                hue, saturation, value = colorsys.rgb_to_hsv(red, green, blue)
                red, green, blue = colorsys.hsv_to_rgb(
                    hue,
                    min(1.0, saturation * 1.197),
                    value * 0.874,
                )
                pixels[base] = red
                pixels[base + 1] = green
                pixels[base + 2] = blue
        result = bpy.data.images.new(
            f"{asset_name}_{view}_HeadBlueGrade",
            width=width,
            height=height,
            alpha=False,
            float_buffer=False,
        )
        result.colorspace_settings.name = "sRGB"
        result.pixels.foreach_set(pixels)
        result.update()
        results[view] = result
    return results


def remap(value: float, source_min: float, source_max: float, target_min: float, target_max: float) -> float:
    ratio = (value - source_min) / max(1e-9, source_max - source_min)
    ratio = max(0.0, min(1.0, ratio))
    return target_min + ratio * (target_max - target_min)


def create_projection_uv_and_assign_faces(mesh_object, crop_bounds: dict, spec: dict) -> dict[str, int]:
    mesh_data = mesh_object.data
    projections = {
        view: mesh_data.uv_layers.new(name=f"Projection{view.title()}")
        for view in ("front", "back", "right")
    }
    xmin, xmax, ymin, ymax, zmin, zmax = mesh_bounds(mesh_object)
    width, depth, height = xmax - xmin, ymax - ymin, zmax - zmin
    center_x = (xmin + xmax) * 0.5
    counts = {"front": 0, "back": 0, "right": 0, "semantic_original": 0, "palette_safe": 0}
    for polygon in mesh_data.polygons:
        normal = polygon.normal
        front_head = False
        mage_hands = False
        # Reserve the side reference for faces genuinely close to +/-X.  A
        # dominant-axis split at 45 degrees puts half of a curved face/torso on
        # the side photo and creates a visible duplicate-face seam.
        if abs(normal.x) > 0.80 and abs(normal.y) < 0.45:
            view = "right"
        elif abs(normal.y) < 0.08:
            view = "front" if polygon.center.y <= (ymin + ymax) * 0.5 else "back"
        else:
            view = "back" if normal.y >= 0.0 else "front"
        if spec["kind"] == "body":
            normalized_z = (polygon.center.z - zmin) / max(1e-9, height)
            normalized_y = (polygon.center.y - ymin) / max(1e-9, depth)
            normalized_abs_x = abs(polygon.center.x - center_x) / max(1e-9, width)
            is_mage = spec.get("palette") == "blue_white_navy"
            if is_mage:
                # Keep the complete hair+face surface in one local projection
                # domain.  The skin/hair boundary is then authored by the view
                # pixels rather than a jagged semantic polygon cut.
                face_y_limit = 0.69
                face_z_min = 0.76
                face_z_max = 1.0
                face_x_limit = 0.22
                front_head = (
                    normalized_z >= face_z_min
                    and normalized_y < face_y_limit
                    and normalized_abs_x < face_x_limit
                )
            else:
                face_y_limit = 0.22
                face_z_min = 0.72
                face_z_max = 0.94
                face_x_limit = 0.205
                front_head = (
                    face_z_min <= normalized_z <= face_z_max
                    and normalized_y < face_y_limit
                    and normalized_abs_x < face_x_limit
                    and (normal.y < -0.22 or abs(normal.x) > 0.62)
                )
            mage_hands = (
                is_mage
                and 0.35 <= normalized_z <= 0.47
                and 0.19 <= normalized_y < 0.42
                and normalized_abs_x > 0.34
            )
            if front_head:
                if not is_mage and (
                    normalized_y < 0.12
                    and normalized_abs_x < 0.14
                    and abs(normal.x) > 0.82
                    and abs(normal.y) < 0.42
                ):
                    polygon.material_index = 2
                    counts["knight_true_profile"] = counts.get("knight_true_profile", 0) + 1
                elif not is_mage and normal.y >= -0.18:
                    # Side/rear hair stays on the skin-free palette.  This is
                    # the explicit guard against a complete second face in 3Q.
                    polygon.material_index = 1
                    counts["knight_head_hair_safe"] = counts.get("knight_head_hair_safe", 0) + 1
                elif is_mage and not (
                    normalized_y < face_y_limit * 0.50 and normal.y < -0.55
                ):
                    # Only genuinely frontal head polygons may see the authored
                    # face.  Every side/rear head polygon is forced to the
                    # skin/feature-free blue back palette, so 3Q can never show
                    # a second eye/mouth stamped from the right turnaround.
                    polygon.material_index = 3
                    counts["head_side_rear_safe"] = counts.get("head_side_rear_safe", 0) + 1
                else:
                    polygon.material_index = 0
                    key = "head_front_original" if is_mage else "head_front_side_original"
                    counts[key] = counts.get(key, 0) + 1
            elif mage_hands:
                polygon.material_index = 2
                counts["hand_original"] = counts.get("hand_original", 0) + 1
            else:
                polygon.material_index = 1
                counts["palette_safe"] += 1
        else:
            polygon.material_index = 0
            counts["semantic_original"] += 1
        counts[view] += 1
        for loop_index in polygon.loop_indices:
            coordinate = mesh_data.vertices[mesh_data.loops[loop_index].vertex_index].co
            front_bounds = crop_bounds["front"]
            if spec.get("palette") == "blue_white_navy" and front_head:
                # Measured full-head bounds in the supplied Mage portrait.  The
                # whole-body crop is asymmetric because a cape/hand reaches the
                # sheet edge, so using that midpoint shifts the face by 14%.
                projections["front"].data[loop_index].uv = (
                    remap(
                        coordinate.x,
                        center_x - width * face_x_limit,
                        center_x + width * face_x_limit,
                        0.449,
                        0.811,
                    ),
                    remap(
                        coordinate.z,
                        zmin + height * face_z_min,
                        zmin + height * face_z_max,
                        0.672,
                        0.893,
                    ),
                )
            elif spec.get("palette") == "blue_white_navy" and mage_hands:
                hand_side = -1.0 if coordinate.x < center_x else 1.0
                radial = remap(
                    abs(coordinate.x - center_x),
                    width * 0.34,
                    width * 0.50,
                    0.0,
                    1.0,
                )
                hand_u = 0.36 - radial * 0.12 if hand_side < 0.0 else 0.92 + radial * 0.078
                projections["front"].data[loop_index].uv = (
                    hand_u,
                    remap(
                        coordinate.z,
                        zmin + height * 0.35,
                        zmin + height * 0.47,
                        0.437,
                        0.507,
                    ),
                )
            else:
                mage_front_shift = 0.137 if spec.get("palette") == "blue_white_navy" else 0.0
                projections["front"].data[loop_index].uv = (
                    remap(coordinate.x, xmin, xmax, front_bounds[0], front_bounds[1])
                    + mage_front_shift,
                    remap(coordinate.z, zmin, zmax, front_bounds[2], front_bounds[3]),
                )
            back_bounds = crop_bounds["back"]
            if spec.get("palette") == "blue_white_navy" and front_head:
                projections["back"].data[loop_index].uv = (
                    remap(
                        -coordinate.x,
                        -(center_x + width * face_x_limit),
                        -(center_x - width * face_x_limit),
                        0.207,
                        0.580,
                    ),
                    remap(
                        coordinate.z,
                        zmin + height * face_z_min,
                        zmin + height * face_z_max,
                        0.672,
                        0.892,
                    ),
                )
            elif spec.get("palette") == "blue_white_navy" and mage_hands:
                radial = remap(
                    abs(coordinate.x - center_x),
                    width * 0.34,
                    width * 0.50,
                    0.0,
                    1.0,
                )
                hand_u = 0.10 - radial * 0.095 if coordinate.x > center_x else 0.68 + radial * 0.10
                projections["back"].data[loop_index].uv = (
                    hand_u,
                    remap(
                        coordinate.z,
                        zmin + height * 0.35,
                        zmin + height * 0.47,
                        0.436,
                        0.500,
                    ),
                )
            else:
                projections["back"].data[loop_index].uv = (
                    remap(-coordinate.x, -xmax, -xmin, back_bounds[0], back_bounds[1]),
                    remap(coordinate.z, zmin, zmax, back_bounds[2], back_bounds[3]),
                )
            right_bounds = crop_bounds["right"]
            # The supplied right/profile turnaround looks from -X, where the
            # character's -Y forward direction is screen-right.  Mirror that
            # same authored profile for +X-facing polygons.
            if normal.x < 0.0:
                side_horizontal, side_min, side_max = -coordinate.y, -ymax, -ymin
            else:
                side_horizontal, side_min, side_max = coordinate.y, ymin, ymax
            if spec.get("palette") == "blue_white_navy" and front_head:
                normalized_loop_y = (coordinate.y - ymin) / max(1e-9, depth)
                profile_u = remap(normalized_loop_y, 0.0, face_y_limit, 0.684, 0.342)
                if normal.x > 0.0:
                    profile_u = 0.342 + 0.684 - profile_u
                projections["right"].data[loop_index].uv = (
                    profile_u,
                    remap(
                        coordinate.z,
                        zmin + height * face_z_min,
                        zmin + height * face_z_max,
                        0.673,
                        0.898,
                    ),
                )
            elif spec.get("palette") == "blue_white_navy" and mage_hands:
                normalized_loop_y = (coordinate.y - ymin) / max(1e-9, depth)
                profile_u = remap(normalized_loop_y, 0.19, 0.42, 0.48, 0.385)
                projections["right"].data[loop_index].uv = (
                    profile_u,
                    remap(
                        coordinate.z,
                        zmin + height * 0.35,
                        zmin + height * 0.47,
                        0.433,
                        0.510,
                    ),
                )
            else:
                projections["right"].data[loop_index].uv = (
                    remap(side_horizontal, side_min, side_max, right_bounds[0], right_bounds[1]),
                    remap(coordinate.z, zmin, zmax, right_bounds[2], right_bounds[3]),
                )
    mesh_data.uv_layers.active = mesh_data.uv_layers["UVMap"]
    mesh_data.update()
    log(f"dominant-normal blended projection and semantic guard counts: {counts}")
    return counts


def mesh_topology_bvh_snapshot(mesh_object) -> dict:
    """Capture topology and disjoint-triangle BVH overlap candidates.

    Pairs which share a vertex are ordinary mesh adjacency and are excluded.
    The overlap set is retained internally so a deformation can be compared to
    the exact pre-deform baseline without rejecting harmless source contacts.
    """
    mesh_data = mesh_object.data
    mesh_data.update()
    bm = bmesh.new()
    bm.from_mesh(mesh_data)
    nonmanifold_edges = sum(1 for edge in bm.edges if not edge.is_manifold)
    boundary_edges = sum(1 for edge in bm.edges if edge.is_boundary)
    bm.free()
    mesh_data.calc_loop_triangles()
    triangles = [tuple(triangle.vertices) for triangle in mesh_data.loop_triangles]
    polygon_indices = [triangle.polygon_index for triangle in mesh_data.loop_triangles]
    coordinates = [vertex.co.copy() for vertex in mesh_data.vertices]
    tree = BVHTree.FromPolygons(
        coordinates,
        triangles,
        all_triangles=True,
        epsilon=1e-7,
    )
    overlaps = set()
    triangle_vertices = [set(triangle) for triangle in triangles]
    for left, right in tree.overlap(tree):
        if left >= right or polygon_indices[left] == polygon_indices[right]:
            continue
        if triangle_vertices[left].isdisjoint(triangle_vertices[right]):
            overlaps.add(tuple(sorted((polygon_indices[left], polygon_indices[right]))))
    return {
        "nonmanifold_edges": nonmanifold_edges,
        "boundary_edges": boundary_edges,
        "degenerate_polygons": sum(
            1 for polygon in mesh_data.polygons if polygon.area < 1e-10
        ),
        "bvh_candidates": overlaps,
    }


def correct_magic_caster_face_geometry(asset_name: str, mesh_object) -> dict:
    """De-puff the Mage head with a topology-safe affine depth compression.

    The earlier lower-face-only push placed skin beneath the bangs and created
    new triangle overlaps.  Instead, face, hair and ears in the accepted head
    projection domain now compress together along Y.  Relative clearance is
    therefore preserved; only a smooth neck-seam fade is non-affine.  UVs were
    authored before this operation and remain unchanged.
    """
    if asset_name != "magic_caster_body":
        return {"moved_vertices": 0, "max_displacement_m": 0.0}

    mesh_data = mesh_object.data
    original_coordinates = [vertex.co.copy() for vertex in mesh_data.vertices]
    original_normals = [polygon.normal.copy() for polygon in mesh_data.polygons]
    original_areas = [polygon.area for polygon in mesh_data.polygons]
    baseline = mesh_topology_bvh_snapshot(mesh_object)

    def smoothstep(edge0: float, edge1: float, value: float) -> float:
        t = max(0.0, min(1.0, (value - edge0) / max(1e-9, edge1 - edge0)))
        return t * t * (3.0 - 2.0 * t)

    # Independently staged and audited against the locked 20:34 mesh.  Above
    # 1.62 m every head layer receives the identical affine transform.  The
    # short lower transition is laterally limited so the neck seam fades to the
    # untouched body/cape without pulling shoulder vertices.
    def head_weight(coordinate) -> float:
        vertical = smoothstep(1.485, 1.585, coordinate.z)
        if vertical <= 0.0:
            return 0.0
        if coordinate.z >= 1.62:
            return 1.0
        lateral = 1.0 - smoothstep(0.285, 0.375, abs(coordinate.x))
        return vertical * lateral

    head_weights = {
        vertex.index: weight
        for vertex in mesh_data.vertices
        if (weight := head_weight(vertex.co)) > 1e-5
    }
    if not head_weights:
        raise RuntimeError("Mage head correction found no head vertices")
    head_y_values = [mesh_data.vertices[index].co.y for index in head_weights]
    head_y_min, head_y_max = min(head_y_values), max(head_y_values)
    depth_scale = 0.88
    moved = 0
    max_displacement = 0.0
    for index, weight in head_weights.items():
        vertex = mesh_data.vertices[index]
        previous = vertex.co.copy()
        local_scale = 1.0 - (1.0 - depth_scale) * weight
        vertex.co.y *= local_scale
        displacement = (vertex.co - previous).length
        if displacement > 1e-7:
            moved += 1
            max_displacement = max(max_displacement, displacement)
    mesh_data.update()

    corrected = mesh_topology_bvh_snapshot(mesh_object)
    new_overlaps = corrected["bvh_candidates"] - baseline["bvh_candidates"]
    flipped_polygon_indices = []
    area_ratios = []
    for index, polygon in enumerate(mesh_data.polygons):
        if original_normals[index].dot(polygon.normal) < 0.0:
            flipped_polygon_indices.append(index)
        if original_areas[index] > 1e-12:
            area_ratios.append(polygon.area / original_areas[index])
    audit = {
        "baseline_nonmanifold_edges": baseline["nonmanifold_edges"],
        "corrected_nonmanifold_edges": corrected["nonmanifold_edges"],
        "baseline_boundary_edges": baseline["boundary_edges"],
        "corrected_boundary_edges": corrected["boundary_edges"],
        "baseline_degenerate_polygons": baseline["degenerate_polygons"],
        "corrected_degenerate_polygons": corrected["degenerate_polygons"],
        "baseline_bvh_candidates": len(baseline["bvh_candidates"]),
        "corrected_bvh_candidates": len(corrected["bvh_candidates"]),
        "new_bvh_candidates": len(new_overlaps),
        "flipped_polygons": len(flipped_polygon_indices),
        "minimum_area_ratio": round(min(area_ratios), 6),
        "maximum_area_ratio": round(max(area_ratios), 6),
    }
    unsafe = (
        corrected["nonmanifold_edges"] != baseline["nonmanifold_edges"]
        or corrected["boundary_edges"] != baseline["boundary_edges"]
        or corrected["degenerate_polygons"] != baseline["degenerate_polygons"]
        or new_overlaps
        or flipped_polygon_indices
        or min(area_ratios) < 0.5
        or max(area_ratios) > 2.0
    )
    if unsafe:
        flipped_details = [
            {
                "index": index,
                "material": mesh_data.polygons[index].material_index,
                "vertices": list(mesh_data.polygons[index].vertices),
                "head_membership": [
                    vertex_index in head_vertex_indices
                    for vertex_index in mesh_data.polygons[index].vertices
                ],
                "center": [round(value, 6) for value in mesh_data.polygons[index].center],
            }
            for index in flipped_polygon_indices
        ]
        for vertex, coordinate in zip(mesh_data.vertices, original_coordinates):
            vertex.co = coordinate
        mesh_data.update()
        raise RuntimeError(
            "Unsafe Mage head correction rejected before bake: "
            + json.dumps(
                {
                    **audit,
                    "new_overlap_pairs": sorted(new_overlaps),
                    "flipped_polygon_indices": flipped_polygon_indices,
                    "flipped_polygon_details": flipped_details,
                }
            )
        )

    log(
        f"topology-safe Mage head compression: {len(head_weights)} head vertices, "
        f"{moved} moved, max {max_displacement:.6f} m; audit={audit}"
    )
    return {
        "method": "semantic_head_affine_depth_compression",
        "head_vertices": len(head_weights),
        "moved_vertices": moved,
        "max_displacement_m": round(max_displacement, 8),
        "depth_scale": depth_scale,
        "head_y_bounds_m": [round(head_y_min, 6), round(head_y_max, 6)],
        "audit": audit,
    }


def projection_material(name: str, source_images: dict, atlas_image, projection_mode: str = "blend"):
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    nodes = material.node_tree.nodes
    nodes.clear()
    output = nodes.new("ShaderNodeOutputMaterial")
    emission = nodes.new("ShaderNodeEmission")
    sources = {}
    for view in ("front", "back", "right"):
        uv = nodes.new("ShaderNodeUVMap")
        uv.uv_map = f"Projection{view.title()}"
        source = nodes.new("ShaderNodeTexImage")
        source.name = f"{view.title()}ProjectionSource"
        source.image = source_images[view]
        source.interpolation = "Linear"
        source.extension = "EXTEND"
        material.node_tree.links.new(uv.outputs["UV"], source.inputs["Vector"])
        sources[view] = source
    target = nodes.new("ShaderNodeTexImage")
    target.name = "BakeTarget"
    target.image = atlas_image
    target.interpolation = "Linear"
    if projection_mode == "front":
        material.node_tree.links.new(sources["front"].outputs["Color"], emission.inputs["Color"])
    elif projection_mode == "back":
        material.node_tree.links.new(sources["back"].outputs["Color"], emission.inputs["Color"])
    elif projection_mode == "right":
        material.node_tree.links.new(sources["right"].outputs["Color"], emission.inputs["Color"])
    elif projection_mode == "face":
        # Kept as an alias for older staged calls, but intentionally front-only:
        # the Mage right turnaround contains full facial features and must never
        # be projected onto a second cheek/hair surface.
        material.node_tree.links.new(sources["front"].outputs["Color"], emission.inputs["Color"])
    else:
        geometry = nodes.new("ShaderNodeNewGeometry")
        separate = nodes.new("ShaderNodeSeparateXYZ")
        abs_x = nodes.new("ShaderNodeMath")
        abs_x.operation = "ABSOLUTE"
        side_weight = nodes.new("ShaderNodeMapRange")
        side_weight.clamp = True
        side_weight.inputs["From Min"].default_value = 0.62
        side_weight.inputs["From Max"].default_value = 0.90
        side_weight.inputs["To Min"].default_value = 0.0
        side_weight.inputs["To Max"].default_value = 1.0
        back_weight = nodes.new("ShaderNodeMapRange")
        back_weight.clamp = True
        back_weight.inputs["From Min"].default_value = -0.65
        back_weight.inputs["From Max"].default_value = 0.65
        back_weight.inputs["To Min"].default_value = 0.0
        back_weight.inputs["To Max"].default_value = 1.0
        front_back_mix = nodes.new("ShaderNodeMixRGB")
        front_back_mix.blend_type = "MIX"
        side_mix = nodes.new("ShaderNodeMixRGB")
        side_mix.blend_type = "MIX"
        material.node_tree.links.new(geometry.outputs["Normal"], separate.inputs["Vector"])
        material.node_tree.links.new(separate.outputs["X"], abs_x.inputs[0])
        material.node_tree.links.new(abs_x.outputs[0], side_weight.inputs["Value"])
        material.node_tree.links.new(separate.outputs["Y"], back_weight.inputs["Value"])
        material.node_tree.links.new(back_weight.outputs["Result"], front_back_mix.inputs["Fac"])
        material.node_tree.links.new(sources["front"].outputs["Color"], front_back_mix.inputs[1])
        material.node_tree.links.new(sources["back"].outputs["Color"], front_back_mix.inputs[2])
        material.node_tree.links.new(side_weight.outputs["Result"], side_mix.inputs["Fac"])
        material.node_tree.links.new(front_back_mix.outputs["Color"], side_mix.inputs[1])
        material.node_tree.links.new(sources["right"].outputs["Color"], side_mix.inputs[2])
        material.node_tree.links.new(side_mix.outputs["Color"], emission.inputs["Color"])
    material.node_tree.links.new(emission.outputs["Emission"], output.inputs["Surface"])
    nodes.active = target
    target.select = True
    return material


def bake_basecolor(
    asset_name: str,
    spec: dict,
    mesh_object,
    source_images: dict,
    texture_path: str,
    atlas_size: int,
):
    atlas = bpy.data.images.new(
        f"{spec['object_name']}_BaseColor_Atlas",
        width=atlas_size,
        height=atlas_size,
        alpha=False,
        float_buffer=False,
    )
    atlas.generated_color = (0.015, 0.015, 0.02, 1.0)
    atlas.colorspace_settings.name = "sRGB"
    palette_safe_images = create_palette_safe_views(asset_name, spec, source_images)
    head_source_images = create_selective_blue_grade_views(asset_name, spec, source_images)
    temporary_materials = []
    original_material = projection_material(
        (
            f"{spec['object_name']}_HeadOriginalTriplanarProjection"
            if spec["kind"] == "body"
            else f"{spec['object_name']}_OriginalProjection"
        ),
        head_source_images,
        atlas,
        projection_mode=("front" if spec["kind"] == "body" else "blend"),
    )
    temporary_materials.append(original_material)
    mesh_object.data.materials.append(original_material)
    if palette_safe_images is not None:
        safe_material = projection_material(
            f"{spec['object_name']}_PaletteSafeProjection",
            palette_safe_images,
            atlas,
        )
        temporary_materials.append(safe_material)
        mesh_object.data.materials.append(safe_material)
        if spec.get("palette") == "blue_white_navy":
            hand_material = projection_material(
                f"{spec['object_name']}_HandOriginalProjection",
                source_images,
                atlas,
            )
            temporary_materials.append(hand_material)
            mesh_object.data.materials.append(hand_material)
            rear_head_material = projection_material(
                f"{spec['object_name']}_RearHeadBackPaletteProjection",
                palette_safe_images,
                atlas,
                projection_mode="back",
            )
            temporary_materials.append(rear_head_material)
            mesh_object.data.materials.append(rear_head_material)
        elif spec.get("palette") == "purple_graphite":
            knight_profile_material = projection_material(
                f"{spec['object_name']}_TrueRightProfileProjection",
                source_images,
                atlas,
                projection_mode="right",
            )
            temporary_materials.append(knight_profile_material)
            mesh_object.data.materials.append(knight_profile_material)
    mesh_object.data.uv_layers.active = mesh_object.data.uv_layers["UVMap"]
    mesh_object.data.uv_layers["UVMap"].active_render = True
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.samples = 1
    scene.cycles.use_denoising = False
    scene.render.bake.margin = max(4, atlas_size // 256)
    scene.render.bake.use_clear = True
    select_only([mesh_object])
    started = time.time()
    bpy.ops.object.bake(
        type="EMIT",
        margin=scene.render.bake.margin,
        margin_type="ADJACENT_FACES",
        use_clear=True,
        target="IMAGE_TEXTURES",
        uv_layer="UVMap",
    )
    os.makedirs(os.path.dirname(texture_path), exist_ok=True)
    atlas.filepath_raw = texture_path
    atlas.file_format = "PNG"
    atlas.save()
    log(f"baked {atlas_size}x{atlas_size} basecolor in {time.time()-started:.1f}s: {texture_path}")

    mesh_object.data.materials.clear()
    for temporary_material in temporary_materials:
        bpy.data.materials.remove(temporary_material, do_unlink=True)
    for layer_name in ("ProjectionFront", "ProjectionBack", "ProjectionRight"):
        projection_layer = mesh_object.data.uv_layers.get(layer_name)
        if projection_layer is not None:
            mesh_object.data.uv_layers.remove(projection_layer)
    final_material = bpy.data.materials.new(spec["object_name"] + "_BaseColor")
    final_material.use_nodes = True
    final_material.diffuse_color = (1.0, 1.0, 1.0, 1.0)
    nodes = final_material.node_tree.nodes
    links = final_material.node_tree.links
    bsdf = next((node for node in nodes if node.type == "BSDF_PRINCIPLED"), None)
    if bsdf is None:
        bsdf = nodes.new("ShaderNodeBsdfPrincipled")
    texture = nodes.new("ShaderNodeTexImage")
    texture.name = "BaseColorAtlas"
    texture.image = atlas
    texture.interpolation = "Linear"
    texture.extension = "EXTEND"
    links.new(texture.outputs["Color"], bsdf.inputs["Base Color"])
    bsdf.inputs["Base Color"].default_value = (1.0, 1.0, 1.0, 1.0)
    bsdf.inputs["Metallic"].default_value = 0.02 if spec["kind"] == "body" else 0.14
    bsdf.inputs["Roughness"].default_value = 0.62 if spec["kind"] == "body" else 0.46
    mesh_object.data.materials.append(final_material)
    for polygon in mesh_object.data.polygons:
        polygon.material_index = 0
    for source_image in source_images.values():
        if source_image.users == 0:
            bpy.data.images.remove(source_image)
    if palette_safe_images is not None:
        for safe_image in palette_safe_images.values():
            if safe_image.users == 0:
                bpy.data.images.remove(safe_image)
    if head_source_images is not source_images:
        for head_image in head_source_images.values():
            if head_image.users == 0:
                bpy.data.images.remove(head_image)
    atlas.pack()
    atlas.filepath = "//" + os.path.basename(texture_path)
    return atlas, final_material


def look_at(obj, target: Vector) -> None:
    obj.rotation_euler = (target - obj.location).to_track_quat("-Z", "Y").to_euler()


def render_qa(asset_name: str, mesh_object, include_three_quarter: bool) -> list[str]:
    os.makedirs(QA_ROOT, exist_ok=True)
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE_NEXT"
    scene.render.resolution_x = 768
    scene.render.resolution_y = 1024
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.film_transparent = False
    scene.view_settings.look = "AgX - Medium High Contrast"
    scene.view_settings.exposure = -0.1
    world = bpy.data.worlds.new(asset_name + "_QAWorld")
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.018, 0.022, 0.032, 1.0)
    world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.28
    scene.world = world

    xmin, xmax, ymin, ymax, zmin, zmax = mesh_bounds(mesh_object)
    width, depth, height = xmax - xmin, ymax - ymin, zmax - zmin
    target = Vector(((xmin + xmax) * 0.5, (ymin + ymax) * 0.5, (zmin + zmax) * 0.5))
    bpy.ops.mesh.primitive_plane_add(
        size=max(5.0, width * 5.0, depth * 5.0),
        location=(target.x, target.y, zmin - 0.006),
    )
    floor = bpy.context.object
    floor.name = asset_name + "_QAFloor"
    floor_material = bpy.data.materials.new(asset_name + "_QAFloorMaterial")
    floor_material.diffuse_color = (0.025, 0.030, 0.042, 1.0)
    floor_material.use_nodes = True
    floor_bsdf = floor_material.node_tree.nodes.get("Principled BSDF")
    floor_bsdf.inputs["Base Color"].default_value = floor_material.diffuse_color
    floor_bsdf.inputs["Roughness"].default_value = 0.72
    floor.data.materials.append(floor_material)

    lights = []
    for index, (location, energy, color, size) in enumerate(
        [
            ((target.x - 3.2, target.y - 4.0, target.z + height * 0.40), 650.0, (1.0, 0.94, 0.88), 3.0),
            ((target.x + 3.5, target.y - 1.2, target.z + height * 0.12), 500.0, (0.76, 0.86, 1.0), 3.2),
            ((target.x, target.y + 3.4, target.z + height * 0.55), 420.0, (0.86, 0.79, 1.0), 2.7),
        ]
    ):
        data = bpy.data.lights.new(f"{asset_name}_QALight{index}", "AREA")
        data.energy = energy
        data.color = color
        data.shape = "DISK"
        data.size = size
        light = bpy.data.objects.new(data.name, data)
        scene.collection.objects.link(light)
        light.location = location
        look_at(light, target)
        lights.append(light)

    camera_data = bpy.data.cameras.new(asset_name + "_QACamera")
    camera = bpy.data.objects.new(camera_data.name, camera_data)
    scene.collection.objects.link(camera)
    camera_data.type = "ORTHO"
    scene.camera = camera
    aspect = scene.render.resolution_x / scene.render.resolution_y
    distance = max(5.0, height * 3.0)
    views = {
        "front": (Vector((target.x, target.y - distance, target.z)), width),
        # Reference right view looks from -X and has the character facing screen-right.
        "side": (Vector((target.x - distance, target.y, target.z)), depth),
        "back": (Vector((target.x, target.y + distance, target.z)), width),
    }
    if include_three_quarter:
        views["three_quarter"] = (
            Vector((target.x - distance * 0.58, target.y - distance, target.z)),
            max(width, depth),
        )
    rendered = []
    for label, (location, horizontal_extent) in views.items():
        camera.location = location
        look_at(camera, target)
        camera_data.ortho_scale = max(
            height * 1.17,
            horizontal_extent / max(0.1, aspect) * 1.18,
        )
        path = os.path.join(QA_ROOT, f"{asset_name}_{label}.png")
        scene.render.filepath = path
        bpy.ops.render.render(write_still=True)
        rendered.append(path)

    scene.camera = None
    scene.world = None
    bpy.data.objects.remove(camera, do_unlink=True)
    for light in lights:
        bpy.data.objects.remove(light, do_unlink=True)
    bpy.data.objects.remove(floor, do_unlink=True)
    bpy.data.materials.remove(floor_material, do_unlink=True)
    bpy.data.worlds.remove(world, do_unlink=True)
    select_only([mesh_object])
    log(f"rendered QA views: {', '.join(rendered)}")
    return rendered


def validate_clean_asset(mesh_object, spec: dict, atlas, atlas_size: int) -> dict:
    xmin, xmax, ymin, ymax, zmin, zmax = mesh_bounds(mesh_object)
    dimensions = [xmax - xmin, ymax - ymin, zmax - zmin]
    components = connected_components(mesh_object.data)
    uv_names = [layer.name for layer in mesh_object.data.uv_layers]
    failures = []
    if len(mesh_object.data.vertices) > spec["vertex_limit"]:
        failures.append("vertex limit exceeded")
    if len(mesh_object.data.polygons) > spec["face_limit"]:
        failures.append("face/triangle limit exceeded")
    if uv_names != ["UVMap"]:
        failures.append(f"unexpected UV layers {uv_names}")
    if len(mesh_object.data.materials) != 1:
        failures.append(f"expected one material, got {len(mesh_object.data.materials)}")
    grip_error = None
    if abs(dimensions[2] - spec["height"]) > 1e-4:
        failures.append(f"height mismatch {dimensions[2]} != {spec['height']}")
    if spec["kind"] == "body":
        if abs(zmin) > 1e-5:
            failures.append(f"body is not grounded: zmin={zmin}")
    else:
        band_min, band_max = spec["grip_band"]
        handle = [
            vertex.co
            for vertex in mesh_object.data.vertices
            if zmin + dimensions[2] * band_min
            <= vertex.co.z
            <= zmin + dimensions[2] * band_max
        ]
        if not handle:
            failures.append("weapon grip audit has no handle vertices")
        else:
            measured_grip = Vector(
                (
                    _median_coordinate([point.x for point in handle]),
                    _median_coordinate([point.y for point in handle]),
                    zmin + dimensions[2] * spec["grip_fraction"],
                )
            )
            grip_error = measured_grip.length
            if grip_error > 0.01:
                failures.append(f"grip origin error {grip_error:.6f} m > 0.01 m")
    if tuple(int(value) for value in atlas.size) != (atlas_size, atlas_size):
        failures.append(f"atlas size mismatch {tuple(atlas.size)}")
    if any(abs(value) > 1e-6 for value in mesh_object.location):
        failures.append(f"location is not applied: {tuple(mesh_object.location)}")
    if any(abs(value - 1.0) > 1e-6 for value in mesh_object.scale):
        failures.append(f"scale is not applied: {tuple(mesh_object.scale)}")
    if failures:
        raise RuntimeError(f"Clean-asset validation failed for {mesh_object.name}: {'; '.join(failures)}")
    return {
        "object": mesh_object.name,
        "vertices": len(mesh_object.data.vertices),
        "faces": len(mesh_object.data.polygons),
        "connected_components": len(components),
        "component_vertices": [len(component) for component in components[:12]],
        "bounds_m": [round(value, 6) for value in dimensions],
        "local_z_bounds_m": [round(zmin, 8), round(zmax, 8)],
        "pivot": "ground" if spec["kind"] == "body" else "grip",
        "grip_origin_error_m": None if grip_error is None else round(grip_error, 8),
        "uv_layers": uv_names,
        "materials": [slot.name for slot in mesh_object.data.materials],
        "atlas": [atlas_size, atlas_size],
    }


def save_and_export(mesh_object, paths: dict) -> None:
    for image in bpy.data.images:
        if image.users and not image.packed_file:
            try:
                image.pack()
            except RuntimeError:
                pass
    bpy.context.preferences.filepaths.save_version = 0
    select_only([mesh_object])
    bpy.ops.wm.save_as_mainfile(filepath=paths["blend"], check_existing=False)
    select_only([mesh_object])
    bpy.ops.export_scene.gltf(
        filepath=paths["glb"],
        check_existing=False,
        export_format="GLB",
        use_selection=True,
        export_materials="EXPORT",
        export_texcoords=True,
        export_normals=True,
        export_tangents=False,
        export_animations=False,
        export_skins=False,
        export_morph=False,
        export_cameras=False,
        export_lights=False,
        export_extras=True,
        export_apply=True,
        export_yup=True,
    )
    log(f"saved clean BLEND {paths['blend']}")
    log(f"exported standalone GLB {paths['glb']}")


def build_asset(asset_name: str, spec: dict, atlas_size: int, include_three_quarter: bool) -> dict:
    started = time.time()
    log(f"=== BUILD {asset_name} ===")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    paths = output_paths(asset_name, spec)
    mesh_object, raw_path = import_joined_mesh(asset_name, spec)
    source_vertices = len(mesh_object.data.vertices)
    cleanup = remove_tiny_components(mesh_object)
    normalized_dimensions = normalize_mesh(mesh_object, spec["height"])
    pre_decimate, final_vertices = decimate_to_game_limits(
        mesh_object,
        spec["vertex_limit"],
        spec["face_limit"],
    )
    # Decimation can perturb the extreme z vertices by tiny floating error; a
    # final normalization guarantees the documented production transform.
    final_dimensions = normalize_mesh(mesh_object, spec["height"])
    grip_audit = place_weapon_grip_at_origin(mesh_object, spec)
    create_clean_uv_map(mesh_object)
    source_images, crop_bounds = load_reference_views(asset_name)
    projection_counts = create_projection_uv_and_assign_faces(mesh_object, crop_bounds, spec)
    geometry_correction = correct_magic_caster_face_geometry(asset_name, mesh_object)
    atlas, _material = bake_basecolor(
        asset_name,
        spec,
        mesh_object,
        source_images,
        paths["texture"],
        atlas_size,
    )
    qa_paths = render_qa(asset_name, mesh_object, include_three_quarter)
    audit = validate_clean_asset(mesh_object, spec, atlas, atlas_size)
    mesh_object["production_v3_source"] = os.path.relpath(raw_path, ROOT).replace("\\", "/")
    mesh_object["production_v3_asset"] = asset_name
    mesh_object["production_v3_kind"] = spec["kind"]
    mesh_object["production_v3_target_height_m"] = spec["height"]
    mesh_object["production_v3_projection"] = "dominant-normal front/back/right -> baked UVMap"
    if spec["kind"] == "weapon":
        mesh_object["production_v3_pivot"] = "grip"
        mesh_object["production_v3_grip_origin_error_m"] = grip_audit["grip_origin_error_m"]
    save_and_export(mesh_object, paths)
    report = {
        "asset": asset_name,
        "source": os.path.relpath(raw_path, ROOT).replace("\\", "/"),
        "source_vertices": source_vertices,
        "cleanup": cleanup,
        "pre_decimate_vertices": pre_decimate,
        "final_vertices": final_vertices,
        "normalized_dimensions_before_decimate": [round(value, 6) for value in normalized_dimensions],
        "normalized_dimensions_final": [round(value, 6) for value in final_dimensions],
        "grip": grip_audit,
        "projection_face_counts": projection_counts,
        "geometry_correction": geometry_correction,
        "projection_crop_uv": {key: [round(v, 6) for v in value] for key, value in crop_bounds.items()},
        "audit": audit,
        "outputs": {
            key: os.path.relpath(value, ROOT).replace("\\", "/")
            for key, value in paths.items()
            if key != "directory"
        },
        "qa": [os.path.relpath(path, ROOT).replace("\\", "/") for path in qa_paths],
        "elapsed_seconds": round(time.time() - started, 2),
    }
    log("BUILD_REPORT=" + json.dumps(report, sort_keys=True))
    return report


def main() -> None:
    options = arguments()
    os.makedirs(QA_ROOT, exist_ok=True)
    reports = []
    failed = []
    for asset_name in options.assets:
        try:
            reports.append(
                build_asset(
                    asset_name,
                    ASSETS[asset_name],
                    options.atlas_size,
                    not options.no_three_quarter,
                )
            )
        except Exception as error:
            failed.append((asset_name, repr(error)))
            log(f"ERROR building {asset_name}: {error!r}")
            import traceback

            traceback.print_exc()
    log("ALL_BUILD_REPORTS=" + json.dumps(reports, sort_keys=True))
    if failed:
        raise RuntimeError(f"Failed assets: {failed}")


if __name__ == "__main__":
    main()
