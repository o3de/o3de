#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

"""Create an editable, two-storey medieval White Box house in the O3DE Editor.

Open a level, then run in the Editor console:
    pyRunFile "<engine root>/Gems/WhiteBox/Editor/Scripts/create_medieval_house.py"

Uses azlmbr.whitebox.api for the walls, roof and framing. The interior staircase is a
parametric Staircase layer on its own "Stairs" entity (through whitebox_author), so its
width, run, height and step count stay editable in the White Box pane; when the authoring
bus is unavailable it falls back to sixteen solid box treads. The authoring bus also adds
material tints. No helper scripts, prefab assets or mesh assets are required. Each run
creates a new house; undo removes it in one step.

Geometry is a collection of closed, editable solids grouped by material. It is
not boolean-unioned and no physics colliders are added. Window panes are opaque
dark panels. The floor joists stop short of the stairwell opening, so nothing hangs
across the stairs. Save the level normally to keep the generated entities.
"""

import math


# All dimensions are metres. Local -Y is the front of the house.
POSITION = (0.0, 0.0, 0.0)
YAW_DEGREES = 0.0
NAME = "Medieval House - Two Storeys"
WIDTH = 8.0
DEPTH = 10.0
STOREY_HEIGHT = 3.0
JETTY = 0.35
ROOF_RISE = 2.5

COLORS = {
    "Stone": (0.46, 0.43, 0.36),
    "Plaster": (0.88, 0.80, 0.63),
    "Timber": (0.23, 0.12, 0.055),
    "Floorboards": (0.42, 0.26, 0.12),
    "Stairs": (0.42, 0.26, 0.12),
    "Roof": (0.34, 0.105, 0.065),
    "Roof Tile Rows": (0.43, 0.15, 0.085),
    "Window Panes": (0.075, 0.12, 0.14),
    "Door and Shutters": (0.32, 0.17, 0.065),
    "Door": (0.32, 0.17, 0.065),
    "Ironwork": (0.07, 0.065, 0.055),
}

# Entities that are children of another batch's entity instead of the house root. The parent
# must come earlier in COLORS, because entities are created in that order.
PARENTS = {"Ironwork": "Door"}


def add(a, b):
    return tuple(a[i] + b[i] for i in range(3))


def sub(a, b):
    return tuple(a[i] - b[i] for i in range(3))


def scale(a, value):
    return tuple(v * value for v in a)


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def length(a):
    return math.sqrt(sum(v * v for v in a))


def normalize(a):
    magnitude = length(a)
    if magnitude < 1e-8:
        raise ValueError("Cannot create a beam with zero length")
    return scale(a, 1.0 / magnitude)


class Geometry:
    """Pure Python positions and outward-wound White Box polygons."""

    def __init__(self):
        self.vertices = []
        self.polygons = []

    def solid(self, points, polygons):
        offset = len(self.vertices)
        self.vertices.extend(points)
        self.polygons.extend(tuple(offset + i for i in face) for face in polygons)

    def box(self, center, size, axes=None):
        if min(size) <= 0:
            raise ValueError("Box dimensions must be positive")
        axes = axes or ((1, 0, 0), (0, 1, 0), (0, 0, 1))
        corners = [(-1, -1, -1), (1, -1, -1), (1, 1, -1), (-1, 1, -1),
                   (-1, -1, 1), (1, -1, 1), (1, 1, 1), (-1, 1, 1)]
        points = []
        for signs in corners:
            point = center
            for axis in range(3):
                point = add(point, scale(axes[axis], signs[axis] * size[axis] / 2))
            points.append(point)
        # Bottom, four walls, then top: no temporarily disconnected vertex fans.
        self.solid(points, [(3, 2, 1, 0), (0, 1, 5, 4), (1, 2, 6, 5),
                            (2, 3, 7, 6), (3, 0, 4, 7), (4, 5, 6, 7)])

    def beam(self, start, end, width=0.16):
        direction = normalize(sub(end, start))
        reference = (0, 0, 1) if abs(direction[2]) < 0.9 else (0, 1, 0)
        x_axis = normalize(cross(reference, direction))
        y_axis = cross(direction, x_axis)
        self.box(scale(add(start, end), 0.5), (width, width, length(sub(end, start))),
                 (x_axis, y_axis, direction))

    def gable(self, y_center, base_z, width, rise, thickness=0.18):
        # X/Z cross-section, extruded along -Y; this basis preserves winding.
        points = []
        for y in (y_center + thickness / 2, y_center - thickness / 2):
            points.extend([(-width / 2, y, base_z), (width / 2, y, base_z),
                           (0, y, base_z + rise)])
        self.solid(points, [(2, 1, 0), (0, 1, 4, 3), (1, 2, 5, 4),
                            (2, 0, 3, 5), (3, 4, 5)])


def rectangles(bounds, holes):
    """Partition a rectangular panel while leaving rectangular openings."""
    u0, u1, v0, v1 = bounds
    clipped = []
    for a, b, c, d in holes:
        a, b, c, d = max(a, u0), min(b, u1), max(c, v0), min(d, v1)
        if b > a and d > c:
            clipped.append((a, b, c, d))
    us = sorted({u0, u1} | {value for hole in clipped for value in hole[:2]})
    vs = sorted({v0, v1} | {value for hole in clipped for value in hole[2:]})
    for a, b in zip(us, us[1:]):
        for c, d in zip(vs, vs[1:]):
            mid_u, mid_v = (a + b) / 2, (c + d) / 2
            if not any(h[0] < mid_u < h[1] and h[2] < mid_v < h[3] for h in clipped):
                yield a, b, c, d


def wall_box(geometry, axis, plane, u, z, width, height, thickness):
    if axis == "x":
        geometry.box((u, plane, z), (width, thickness, height))
    else:
        geometry.box((plane, u, z), (thickness, width, height))


def wall(geometry, axis, plane, span, base, top, thickness, holes):
    for a, b, c, d in rectangles((-span / 2, span / 2, base, top), holes):
        wall_box(geometry, axis, plane, (a + b) / 2, (c + d) / 2,
                 b - a, d - c, thickness)


def window(batches, axis, outside, outward, hole, shutters=False):
    a, b, c, d = hole
    center_u, center_z = (a + b) / 2, (c + d) / 2
    frame_plane = outside + outward * 0.055
    wall_box(batches["Window Panes"], axis, outside - outward * 0.06,
             center_u, center_z, b - a, d - c, 0.035)
    for u in (a - 0.04, b + 0.04, center_u):
        wall_box(batches["Timber"], axis, frame_plane, u, center_z,
                 0.085, d - c + 0.16, 0.12)
    for z in (c - 0.04, d + 0.04, center_z):
        wall_box(batches["Timber"], axis, frame_plane, center_u, z,
                 b - a + 0.16, 0.085, 0.12)
    if shutters:
        for u in (a - 0.22, b + 0.22):
            wall_box(batches["Door and Shutters"], axis, frame_plane,
                     u, center_z, 0.25, d - c, 0.07)


FLOOR_BASE = 0.6  # Top of the ground floor; the stairs start here.
STAIR_STEPS = 16
STAIR_WIDTH = 1.5
STAIR_OPENING_MARGIN = 0.10  # Clear space beside the stairs' open side in the upper floor opening.
STAIR_WALL_OVERLAP = 0.02  # The stairs' wall side sinks this far into the wall so no gap shows.
GROUND_WALL_THICKNESS = 0.28
# Surfaces of different materials that lie in the same plane z-fight (flicker). Nudge one of them by
# at least this much so no two visible faces of different batches coincide.
Z_FIGHT_GAP = 0.02
RAIL_HEIGHT = 0.90  # Handrail height above the line through the stair nosings.
RAIL_POST = 0.12
WALL_RAIL_SIZE = 0.06
WALL_RAIL_CLEARANCE = 0.12  # Gap between the wall handrail and the sill of a window above it.
WALL_RAIL_OFFSET = 0.07  # Wall handrail centre line, from the wall face.
JOIST_THICKNESS = 0.18  # Along the joist's short horizontal axis (Y).


def stair_layout(width=WIDTH, depth=DEPTH, storey_height=STOREY_HEIGHT):
    """Where the interior staircase sits, shared by the opening, the joists and the stairs.

    The staircase climbs toward +Y from the ground floor to the upper floor, with its left
    side against the inside face of the left wall. "x" is its centre line, "run" its length
    along Y and "hole" the (x0, x1, y0, y1) opening it needs in the upper floor: it starts at
    the wall and leaves a margin on the open side only.
    """
    wall_inner = -width / 2 + GROUND_WALL_THICKNESS
    x = wall_inner + STAIR_WIDTH / 2 - STAIR_WALL_OVERLAP
    run = depth * 0.60
    return {
        "x": x,
        "run": run,
        "width": STAIR_WIDTH,
        "rise": storey_height,
        "steps": STAIR_STEPS,
        "base": FLOOR_BASE,
        "wall_inner": wall_inner,
        "hole": (wall_inner - Z_FIGHT_GAP, x + STAIR_WIDTH / 2 + STAIR_OPENING_MARGIN, -run / 2, run / 2),
    }


def nosing_z(stairs, y):
    """Height of the line through the stair nosings (front top edge of each tread) at y."""
    y0 = -stairs["run"] / 2
    return stairs["base"] + stairs["rise"] / stairs["steps"] + stairs["rise"] * (y - y0) / stairs["run"]


def stair_top_z(stairs, y):
    """Top of the solid staircase at y, or the ground floor when y is outside its run."""
    y0 = -stairs["run"] / 2
    if y <= y0:
        return stairs["base"]
    step = min(stairs["steps"], int((y - y0) / (stairs["run"] / stairs["steps"])) + 1)
    return stairs["base"] + stairs["rise"] * step / stairs["steps"]


def stair_side_windows(stairs, depth, base, upper):
    """Ground-floor windows of the wall the stairs run along, as (y0, y1, z0, z1) holes.

    A window the solid stairs cover is dropped. One the wall handrail would cross is raised so the
    handrail passes under its sill.
    """
    windows = []
    for y in (-depth * 0.25, depth * 0.25):
        y0, y1, z0, z1 = y - 0.48, y + 0.48, base + 1.1, base + 2.2
        if stair_top_z(stairs, y1) > z0:
            continue
        rail_top = nosing_z(stairs, y1) + RAIL_HEIGHT + WALL_RAIL_SIZE / 2
        if rail_top + WALL_RAIL_CLEARANCE > z0:
            z0 = rail_top + WALL_RAIL_CLEARANCE
            z1 = min(z0 + 1.0, upper - 0.35)
        windows.append((y0, y1, z0, z1))
    return windows


def stair_rails(timber, stairs, upper):
    """Balustrade on the open (+X) side and a wall-mounted handrail on the wall side.

    Both follow the line through the stair nosings, raised by RAIL_HEIGHT. The balustrade has
    a newel post at each end, a handrail and one baluster per tread; the wall rail is held off
    the wall by brackets.
    """
    x, run, rise, steps, base = stairs["x"], stairs["run"], stairs["rise"], stairs["steps"], stairs["base"]
    tread = run / steps
    y0, y1 = -run / 2, run / 2

    def line(y):
        return nosing_z(stairs, y)

    open_x = x + stairs["width"] / 2 - RAIL_POST / 2 - 0.01
    first, last = y0 + RAIL_POST / 2, y1 - RAIL_POST / 2
    timber.beam((open_x, first, line(first) + RAIL_HEIGHT), (open_x, last, line(last) + RAIL_HEIGHT), 0.07)
    # Newel posts, from the tread (bottom) or the upper floor (top) up past the handrail.
    timber.box((open_x, first, (base + line(first) + RAIL_HEIGHT + 0.10) / 2),
               (RAIL_POST, RAIL_POST, line(first) + RAIL_HEIGHT + 0.10 - base))
    timber.box((open_x, last, (upper + line(last) + RAIL_HEIGHT + 0.10) / 2),
               (RAIL_POST, RAIL_POST, line(last) + RAIL_HEIGHT + 0.10 - upper))
    for step in range(1, steps - 1):
        y = y0 + (step + 0.5) * tread
        top_of_tread = base + rise * (step + 1) / steps
        timber.beam((open_x, y, top_of_tread - 0.02), (open_x, y, line(y) + RAIL_HEIGHT - 0.035), 0.04)

    wall_x = stairs["wall_inner"] + WALL_RAIL_OFFSET
    timber.beam((wall_x, y0, line(y0) + RAIL_HEIGHT), (wall_x, y1, line(y1) + RAIL_HEIGHT), WALL_RAIL_SIZE)
    for i in range(4):
        y = y0 + (i + 0.5) * run / 4
        timber.box(((stairs["wall_inner"] + wall_x) / 2, y, line(y) + RAIL_HEIGHT - 0.07),
                   (WALL_RAIL_OFFSET, 0.05, 0.12))


def floor_joists(timber, depth, upper, upper_width, hole):
    """Exposed joists that run across the house but stop at the stairwell opening.

    A joist whose width overlaps the opening in Y is split into a left and a right stub, so
    no timber crosses the stairs below the upper floor.
    """
    x0, x1, y0, y1 = hole
    half = JOIST_THICKNESS / 2
    left, right = -(upper_width + 0.12) / 2, (upper_width + 0.12) / 2
    for i in range(10):
        y = -depth / 2 + (i + 0.5) * depth / 10
        crosses = y + half > y0 and y - half < y1
        for a, b in ([(left, x0 - Z_FIGHT_GAP), (x1 + Z_FIGHT_GAP, right)] if crosses else [(left, right)]):
            if b - a > 1e-6:
                timber.box(((a + b) / 2, y, upper - 0.24), (b - a, JOIST_THICKNESS, 0.20))


def make_geometry(width=WIDTH, depth=DEPTH, storey_height=STOREY_HEIGHT,
                  jetty=JETTY, roof_rise=ROOF_RISE, parametric_stairs=False):
    """Return grouped geometry without accessing or changing the Editor.

    With parametric_stairs the "Stairs" batch is left empty: the caller builds a Staircase
    layer from stair_layout() instead. Without it the stairs are sixteen solid box treads.
    """
    if not all(math.isfinite(v) for v in (width, depth, storey_height, jetty, roof_rise)):
        raise ValueError("Dimensions must be finite")
    if width < 6 or depth < 8 or storey_height < 2.6 or jetty < 0 or roof_rise < 1:
        raise ValueError("Use width >= 6, depth >= 8, storey height >= 2.6, jetty >= 0 and roof rise >= 1")
    batches = {name: Geometry() for name in COLORS}
    stone, plaster, timber = batches["Stone"], batches["Plaster"], batches["Timber"]
    wood = batches["Floorboards"]
    base = FLOOR_BASE
    upper = base + storey_height
    stone_top = upper - 0.01  # Under the floorboards, whose top face is at upper.
    eaves = upper + storey_height
    upper_width, upper_depth = width + 2 * jetty, depth + 2 * jetty

    stone.box((0, 0, 0.2), (width + 0.15, depth + 0.15, 0.4))
    wood.box((0, 0, 0.5), (width, depth, 0.2))
    for step in range(3):
        height = (step + 1) * 0.2
        stone.box((0, -depth / 2 - 0.75 + 0.3 * step, height / 2), (2, 0.3, height))

    # The upper floor has a real opening above the interior staircase.
    stairs = stair_layout(width, depth, storey_height)
    stair_x, stair_run, stair_hole = stairs["x"], stairs["run"], stairs["hole"]
    for a, b, c, d in rectangles((-upper_width / 2, upper_width / 2,
                                 -upper_depth / 2, upper_depth / 2), [stair_hole]):
        wood.box(((a + b) / 2, (c + d) / 2, upper - 0.1), (b - a, d - c, 0.2))

    ground_front_windows = [(x - 0.48, x + 0.48, base + 1.1, base + 2.2)
                            for x in (-width * 0.32, width * 0.32)]
    door = (-0.72, 0.72, base, base + 2.3)
    front_outside = -depth / 2
    wall(stone, "x", front_outside + GROUND_WALL_THICKNESS / 2, width, base, stone_top, GROUND_WALL_THICKNESS,
         ground_front_windows + [door])
    for hole in ground_front_windows:
        window(batches, "x", front_outside, -1, hole, shutters=True)
    for side in (-1, 1):
        if side == -1:
            side_windows = stair_side_windows(stairs, depth, base, upper)
        else:
            side_windows = [(y - 0.48, y + 0.48, base + 1.1, base + 2.2)
                            for y in (-depth * 0.25, depth * 0.25)]
        wall(stone, "y", side * (width / 2 - GROUND_WALL_THICKNESS / 2), depth, base, stone_top,
             GROUND_WALL_THICKNESS, side_windows)
        for hole in side_windows:
            window(batches, "y", side * width / 2, side, hole)
    wall(stone, "x", depth / 2 - GROUND_WALL_THICKNESS / 2, width, base, stone_top,
         GROUND_WALL_THICKNESS, ground_front_windows)
    for hole in ground_front_windows:
        window(batches, "x", depth / 2, 1, hole)

    # Planked door (its own entity, parent of the ironwork), timber surround, iron straps and handle.
    for plank in range(7):
        wall_box(batches["Door"], "x", front_outside + 0.035,
                 -0.70 + (plank + 0.5) * 0.20, base + 1.15, 0.19, 2.3, 0.08)
    for x in (-0.80, 0.80):
        # Its inner face sits just inside the door opening instead of in the plane of the wall reveal.
        timber.box((x * 0.98, front_outside - 0.04, base + 1.15), (0.16, 0.18, 2.46))
    timber.box((0, front_outside - 0.04, base + 2.38 - Z_FIGHT_GAP), (1.76, 0.18, 0.16))
    for z in (base + 0.5, base + 1.8):
        batches["Ironwork"].box((0, front_outside - 0.025, z), (1.3, 0.045, 0.08))
    batches["Ironwork"].box((0.48, front_outside - 0.09, base + 1.0), (0.09, 0.08, 0.14))

    for side in (-1, 1):
        outside_y = side * upper_depth / 2
        front_windows = [(x - 0.40, x + 0.40, upper + 1.0, upper + 2.1)
                         for x in (-upper_width * 0.375, -upper_width * 0.125,
                                   upper_width * 0.125, upper_width * 0.375)]
        wall(plaster, "x", outside_y - side * 0.10, upper_width,
             upper, eaves, 0.20, front_windows)
        for hole in front_windows:
            window(batches, "x", outside_y, side, hole, shutters=True)
        posts = [-upper_width / 2 + i * upper_width / 4 for i in range(5)]
        for x in posts:
            # A little taller than the wall so the post's ends are not in the wall's top and bottom planes.
            timber.box((x, outside_y + side * 0.04, (upper + eaves) / 2),
                       (0.17, 0.18, storey_height + 2 * Z_FIGHT_GAP))
        for z in (upper + 0.10, upper + 0.85, eaves - 0.10):
            timber.box((0, outside_y + side * 0.04, z), (upper_width + 0.16, 0.18, 0.18))
        for a, b in zip(posts, posts[1:]):
            y = outside_y + side * 0.055
            timber.beam((a + 0.14, y, upper + 0.22), (b - 0.14, y, upper + 0.73), 0.115)
            timber.beam((a + 0.14, y, upper + 0.73), (b - 0.14, y, upper + 0.22), 0.115)

    for side in (-1, 1):
        outside_x = side * upper_width / 2
        holes = [(y - 0.44, y + 0.44, upper + 1.0, upper + 2.1)
                 for y in (-upper_depth * 0.375, -upper_depth * 0.125,
                           upper_depth * 0.125, upper_depth * 0.375)]
        wall(plaster, "y", outside_x - side * 0.10, upper_depth, upper, eaves, 0.20, holes)
        for hole in holes:
            window(batches, "y", outside_x, side, hole)
        for i in range(5):
            y = -upper_depth / 2 + i * upper_depth / 4
            timber.box((outside_x + side * 0.04, y, (upper + eaves) / 2),
                       (0.18, 0.17, storey_height + 2 * Z_FIGHT_GAP))
        for z in (upper + 0.10, upper + 0.85, eaves - 0.10):
            timber.box((outside_x + side * 0.04, 0, z), (0.18, upper_depth + 0.16, 0.18))

    # Exposed floor joists emphasize the cantilevered upper storey. They stop at the
    # stairwell opening so they never cross the stairs.
    floor_joists(timber, depth, upper, upper_width, stair_hole)

    # Fallback stairs: sixteen solid treads. Either way the open side gets a balustrade.
    if not parametric_stairs:
        for step in range(stairs["steps"]):
            rise = storey_height * (step + 1) / stairs["steps"]
            y = -stair_run / 2 + (step + 0.5) * stair_run / stairs["steps"]
            batches["Stairs"].box((stair_x, y, base + rise / 2),
                                  (stairs["width"], stair_run / stairs["steps"], rise))
    stair_rails(timber, stairs, upper)

    ridge_z = eaves + roof_rise
    for side in (-1, 1):
        y = side * (upper_depth / 2 - 0.08)
        plaster.gable(y, eaves, upper_width, roof_rise)
        face_y = side * (upper_depth / 2 + 0.05)
        timber.beam((-upper_width / 2, face_y, eaves), (0, face_y, ridge_z), 0.18)
        timber.beam((0, face_y, ridge_z), (upper_width / 2, face_y, eaves), 0.18)
        timber.beam((0, face_y, eaves), (0, face_y, ridge_z), 0.16)
        timber.box((0, face_y, eaves), (upper_width + 0.18, 0.18, 0.20))

    # Two pitched roof slabs plus raised tile courses and ridge trim.
    roof_half = upper_width / 2 + 0.45
    roof_depth = upper_depth + 0.95
    roof_edge_z = ridge_z - roof_rise * roof_half / (upper_width / 2)
    for side in (-1, 1):
        start, end = ((-roof_half, 0, roof_edge_z), (0, 0, ridge_z)) if side < 0 else (
            (0, 0, ridge_z), (roof_half, 0, roof_edge_z))
        slope = normalize(sub(end, start))
        normal = cross(slope, (0, 1, 0))
        slope_length = length(sub(end, start))
        center = add(scale(add(start, end), 0.5), scale(normal, 0.06))
        axes = (slope, (0, 1, 0), normal)
        batches["Roof"].box(center, (slope_length, roof_depth, 0.12), axes)
        for row in range(12):
            t = (row + 0.5) / 12
            row_center = add(add(start, scale(sub(end, start), t)), scale(normal, 0.14))
            batches["Roof Tile Rows"].box(row_center,
                                         (slope_length / 12 - 0.025, roof_depth, 0.055), axes)
    timber.box((0, 0, ridge_z + 0.13), (0.20, roof_depth + 0.16, 0.20))

    # Hollow masonry chimney and a four-piece cap rim.
    chimney_x, chimney_y = width * 0.28, depth * 0.20
    chimney_base, chimney_top = eaves - 0.2, ridge_z + 0.65
    chimney_h = chimney_top - chimney_base
    for sign in (-1, 1):
        stone.box((chimney_x + sign * 0.35, chimney_y, chimney_base + chimney_h / 2),
                  (0.18, 0.95, chimney_h))
        stone.box((chimney_x, chimney_y + sign * 0.385, chimney_base + chimney_h / 2),
                  (0.52, 0.18, chimney_h))
        stone.box((chimney_x + sign * 0.40, chimney_y, chimney_top), (0.22, 1.15, 0.16))
        stone.box((chimney_x, chimney_y + sign * 0.465, chimney_top), (0.58, 0.22, 0.16))
    return batches


def _load_whitebox_author():
    """Import whitebox_author (the parametric layer API), or return None if it is unusable.

    It lives next to this script in the WhiteBox gem's Editor/Scripts and raises
    RuntimeError on import when the gem was built without the authoring bus.
    """
    try:
        import whitebox_author
    except ImportError:
        try:
            import os
            import sys
            sys.path.append(os.path.dirname(os.path.abspath(__file__)))
            import whitebox_author
        except (ImportError, NameError, RuntimeError):
            return None
    except RuntimeError:
        return None
    return whitebox_author


def _build_parametric_stairs(author, child, layout):
    """Turn the empty White Box on child into a Staircase layer matching layout."""
    stairs = author.WhiteBox.existing(child)
    stairs.clear()
    index = stairs.add_shape("staircase", width=layout["width"], depth=layout["run"],
                             height=layout["rise"], steps=layout["steps"])
    stairs.set_layer(index, name="Stairs")


def create_medieval_house(position=POSITION, yaw_degrees=YAW_DEGREES, name=NAME):
    """Create the hierarchy and return the root EntityId. Run inside the Editor."""
    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.entity as entity
    import azlmbr.math as amath
    import azlmbr.components as components

    # O3DE's reflected constructor expects three Python floats, including zeros.
    def vec3(values):
        return amath.Vector3(float(values[0]), float(values[1]), float(values[2]))
    import azlmbr.whitebox.api  # Registers WhiteBoxMeshHandle Python methods.
    import azlmbr.whitebox.request.bus as wb_request
    import azlmbr.whitebox.notification.bus as wb_notification
    try:
        import azlmbr.whitebox.authoring as authoring
    except ImportError:
        authoring = None

    author = _load_whitebox_author()
    layout = stair_layout()
    batches = make_geometry(parametric_stairs=author is not None)
    types = editor.EditorComponentAPIBus(
        bus.Broadcast, "FindComponentTypeIdsByEntityType", ["White Box"], entity.EntityType().Game)
    if not types:
        raise RuntimeError("White Box is unavailable. Enable the WhiteBox gem and open an Editor level.")

    root = None
    named_entities = []
    editor.ToolsApplicationRequestBus(bus.Broadcast, "BeginUndoBatch", "Create two-storey medieval house")
    try:
        root = editor.ToolsApplicationRequestBus(bus.Broadcast, "CreateNewEntity", entity.EntityId())
        if not root.IsValid():
            raise RuntimeError("Could not create the house root. Open a level first.")
        named_entities.append((root, name))
        components.TransformBus(bus.Event, "SetLocalTranslation", root, vec3((0.0, 0.0, 0.0)))
        components.TransformBus(bus.Event, "SetLocalRotation", root, vec3((0.0, 0.0, 0.0)))
        components.TransformBus(bus.Event, "SetLocalUniformScale", root, 1.0)

        created = {}
        for label, geometry in batches.items():
            child = editor.ToolsApplicationRequestBus(bus.Broadcast, "CreateNewEntity",
                                                      created.get(PARENTS.get(label), root))
            if not child.IsValid():
                raise RuntimeError("Could not create the %s entity" % label)
            named_entities.append((child, label))
            created[label] = child
            components.TransformBus(bus.Event, "SetLocalTranslation", child, vec3((0.0, 0.0, 0.0)))
            components.TransformBus(bus.Event, "SetLocalRotation", child, vec3((0.0, 0.0, 0.0)))
            components.TransformBus(bus.Event, "SetLocalUniformScale", child, 1.0)
            outcome = editor.EditorComponentAPIBus(bus.Broadcast, "AddComponentsOfType", child, [types[0]])
            if not outcome.IsSuccess():
                raise RuntimeError("Could not add White Box to %s" % label)
            added = outcome.GetValue()
            if not added:
                raise RuntimeError("No White Box component returned for %s" % label)
            editor.EditorComponentAPIBus(bus.Broadcast, "EnableComponents", added)
            component = added[0]  # EntityComponentIdPair; do not substitute just the EntityId.
            if label == "Stairs" and author is not None:
                # The entity sits at the foot of the stairs, so the layer stays at its own origin.
                components.TransformBus(bus.Event, "SetLocalTranslation", child,
                                        vec3((layout["x"], 0.0, layout["base"])))
                _build_parametric_stairs(author, child, layout)
            else:
                mesh = wb_request.EditorWhiteBoxComponentRequestBus(bus.Event, "GetWhiteBoxMeshHandle", component)
                if not mesh.IsValid():
                    raise RuntimeError("White Box mesh handle is invalid for %s" % label)
                mesh.Clear()
                vertices = [mesh.AddVertex(vec3(point)) for point in geometry.vertices]
                for polygon in geometry.polygons:
                    handles = [vertices[i] for i in polygon]
                    if len(handles) == 4:
                        mesh.AddQuadPolygon(*handles)
                    else:
                        mesh.AddTriPolygon(*handles)
                expected_faces = sum(len(polygon) - 2 for polygon in geometry.polygons)
                if mesh.MeshFaceCount() != expected_faces or mesh.MeshVertexCount() != len(vertices):
                    raise RuntimeError("White Box rejected the closed-solid topology for %s" % label)
                mesh.CalculateNormals()
                mesh.CalculatePlanarUVs()
                wb_request.EditorWhiteBoxComponentRequestBus(bus.Event, "SerializeWhiteBox", component)
                wb_notification.EditorWhiteBoxComponentNotificationBus(bus.Event, "OnWhiteBoxMeshModified", component)
            if authoring is not None:
                settings = authoring.EditorWhiteBoxAuthoringRequestBus(bus.Event, "GetDisplaySettings", child)
                settings.UseGlobalTint = True
                settings.UseTexture = False
                settings.EdgesOnly = False
                settings.Tint = amath.Color(*(float(value) for value in COLORS[label]), 1.0)
                result = authoring.EditorWhiteBoxAuthoringRequestBus(bus.Event, "SetDisplaySettings", child, settings)
                if not result.Success:
                    raise RuntimeError("Could not tint %s: %s" % (label, result.Message))
            editor.ToolsApplicationRequestBus(bus.Broadcast, "AddDirtyEntity", child)

        # Set names after entity creation, when prefab DOM reconciliation is finished.
        for eid, label in named_entities:
            editor.EditorEntityAPIBus(bus.Event, "SetName", eid, label)
            editor.ToolsApplicationRequestBus(bus.Broadcast, "AddDirtyEntity", eid)
        components.TransformBus(bus.Event, "SetLocalTranslation", root, vec3(position))
        components.TransformBus(bus.Event, "SetLocalRotation", root,
                                vec3((0.0, 0.0, math.radians(float(yaw_degrees)))))
        editor.ToolsApplicationRequestBus(bus.Broadcast, "AddDirtyEntity", root)
        editor.ToolsApplicationRequestBus(bus.Broadcast, "SetSelectedEntities", [root])
    except Exception:
        if root is not None and root.IsValid():
            editor.ToolsApplicationRequestBus(bus.Broadcast, "DeleteEntitiesAndAllDescendants", [root])
        raise
    finally:
        editor.ToolsApplicationRequestBus(bus.Broadcast, "EndUndoBatch")

    if authoring is None:
        print("Created geometry with default White Box materials; the optional authoring tint API is unavailable.")
    if author is None:
        print("The Stairs entity uses box treads: whitebox_author is unavailable, so there is no parametric Staircase layer.")
    print("Created %s: two floors, internal stairs, timber framing, gable roof and chimney. Root: %s" %
          (name, root.ToString()))
    return root


if __name__ == "__main__":
    create_medieval_house()
