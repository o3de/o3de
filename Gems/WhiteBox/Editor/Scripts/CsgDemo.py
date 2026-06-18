"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Usage
-----
    pyRunFile path/to/CsgDemo.py [doorway|ledge|chamfer]

Each mode runs a single CSG subtract on a unit-cube source brush,
demonstrating a shape you cannot easily build with WhiteBox's native
extrude/translate-polygon operations.

NOTE on operands
----------------
MeshBoolean is backed by the Manifold library, which is robust for any
closed (watertight) two-manifold operands, convex or not - including
meshes that have already been CSG-subtracted.  The historical convex-only
constraint of the old half-space clipper no longer applies.

Modes
-----
doorway  – punches a rectangular slot through a thick wall block, producing
           the cross-section of a door-frame jamb.  Useful for arches and
           openings in walls.

ledge    – carves a step-notch from one corner to create a shelving ledge
           or a coping stone profile.

chamfer  – carves a 45-degree angled corner using a rotated cutter, giving
           a bevelled edge that native extrusion cannot produce directly.
"""

import math
import WhiteBoxInit as init
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.math

SUBTRACTION = 1

V3  = azlmbr.math.Vector3
Tf  = azlmbr.math.Transform_CreateTranslation
TfS = azlmbr.math.Transform_CreateUniformScale
TfR = azlmbr.math.Transform_CreateRotationY


# ── helpers ───────────────────────────────────────────────────────────────────

def compose(translation=None, scale=1.0, rot_y=0.0, rot_z=0.0):
    """
    Build a single cutter transform.

    azlmbr's Transform proxy does NOT expose the `*` operator, so
    transforms cannot be multiplied in Python.  But a translation,
    uniform scale and a single-axis rotation always collapse into one
    Transform, because TransformPoint(p) = translation + scale * (rotation * p),
    which is exactly translate(scale(rotate(p))).

        translation – V3 applied last (cutter position, world-equiv)
        scale       – uniform scale factor
        rot_y       – rotation about Y in radians (bevels an X-Z / horizontal edge)
        rot_z       – rotation about Z in radians (bevels an X-Y / vertical edge)

    Pass at most one of rot_y / rot_z (rot_z wins if both are given).
    """
    if rot_z != 0.0:
        quat = azlmbr.math.Quaternion_CreateRotationZ(rot_z)
    else:
        quat = azlmbr.math.Quaternion_CreateRotationY(rot_y)
    pos  = translation if translation is not None else V3(0.0, 0.0, 0.0)
    tf   = azlmbr.math.Transform_CreateFromQuaternionAndTranslation(quat, pos)
    tf.SetUniformScale(scale)
    return tf

def make_cube(name):
    entity    = init.create_white_box_entity(name)
    component = init.create_white_box_component(entity)
    handle    = init.create_white_box_handle(component)
    # The White Box component already initializes its mesh as a default unit
    # cube, and InitializeAsUnitCube APPENDS rather than replaces - so calling
    # it without clearing first leaves two coincident cubes (16 verts / 24
    # faces).  That doubled, non-manifold mesh makes the CSG boolean produce
    # garbage (diagonal cuts, holes, inverted faces).  Clear first.
    handle.Clear()
    handle.InitializeAsUnitCube()
    return entity, component, handle


def make_box(name, center, half):
    """
    Build a box cutter of arbitrary (non-uniform) dimensions.

    AZ::Transform only supports UNIFORM scale, so a scaled unit cube can only
    ever be a cube - useless for a tall, narrow, deep door slot.  Instead we
    build the eight corners directly with AddVertex and stitch the six quads
    with the same winding the engine's InitializeAsUnitCube uses (so the
    cutter is a valid closed manifold for the boolean).

        center – (x, y, z) box centre
        half   – (hx, hy, hz) half-extents on each axis
    """
    entity    = init.create_white_box_entity(name)
    component = init.create_white_box_component(entity)
    handle    = init.create_white_box_handle(component)
    handle.Clear()

    cx, cy, cz = center
    hx, hy, hz = half
    x0, x1 = cx - hx, cx + hx
    y0, y1 = cy - hy, cy + hy
    z0, z1 = cz - hz, cz + hz

    v = [
        handle.AddVertex(V3(x0, y0, z1)),  # 0  top
        handle.AddVertex(V3(x1, y0, z1)),  # 1
        handle.AddVertex(V3(x1, y1, z1)),  # 2
        handle.AddVertex(V3(x0, y1, z1)),  # 3
        handle.AddVertex(V3(x0, y0, z0)),  # 4  bottom
        handle.AddVertex(V3(x1, y0, z0)),  # 5
        handle.AddVertex(V3(x1, y1, z0)),  # 6
        handle.AddVertex(V3(x0, y1, z0)),  # 7
    ]
    handle.AddQuadPolygon(v[0], v[1], v[2], v[3])  # top    (+Z)
    handle.AddQuadPolygon(v[7], v[6], v[5], v[4])  # bottom (-Z)
    handle.AddQuadPolygon(v[4], v[5], v[1], v[0])  # front  (-Y)
    handle.AddQuadPolygon(v[5], v[6], v[2], v[1])  # right  (+X)
    handle.AddQuadPolygon(v[6], v[7], v[3], v[2])  # back   (+Y)
    handle.AddQuadPolygon(v[7], v[4], v[0], v[3])  # left   (-X)
    handle.CalculateNormals()
    handle.CalculatePlanarUVs()
    return entity, component, handle


def csg_subtract(target_handle, target_component, cutter_entity, cutter_handle, transform):
    """
    Subtract cutter from target in place, then delete the cutter entity.

    transform  – positions/sizes the cutter relative to the source brush's
                 local origin (the centre of the unit cube, i.e. [0,0,0]).
                   Tf(V3(x,y,z))          pure translation
                   TfS(s)                 uniform scale  (makes cutter s× bigger)
                   TfR(angle_radians)     rotation around Y
                 Compose two transforms by multiplying: TfR(a) * Tf(v)

    The source brush must be a closed (watertight) mesh; it does not
    need to be convex.
    """
    if target_handle.MeshBoolean(cutter_handle, transform, SUBTRACTION):
        init.update_white_box(target_handle, target_component)
        editor.ToolsApplicationRequestBus(bus.Broadcast, 'DeleteEntityById', cutter_entity)
        return True
    print('csg_subtract: no intersection — check that the meshes overlap')
    return False


# ── modes ─────────────────────────────────────────────────────────────────────

def demo_doorway():
    """
    A doorway punched through the FRONT face of a wall block.

    Source:  unit cube (a thick wall section)
    Cutter:  a door-shaped box - narrow in x, tall in z, and longer than the
             wall in y so it tunnels all the way through front-to-back.
    Result:  wall block with a doorway opening: jambs either side, a lintel
             across the top, open to the floor.

    Viewed from the front (x-z plane):
        ┌──┬──┬──┐
        │  │  │  │    ← lintel above the opening
        │  │  │  │    ← jambs either side
        │  │  │  │    ← opening, open to the floor
        └──┘  └──┘
    """
    entity, comp, wb = make_cube('WallJamb')
    # Door cutter (local space, wall centred on origin, z = up):
    #   x: half 0.2  -> 0.4 wide opening, centred
    #   y: half 0.6  -> longer than the 1.0-deep wall so it punches clean
    #                   through both faces (no coplanar faces with the wall)
    #   z: spans from below the floor (-0.6) up to 0.25 -> a 0.75-tall opening
    #      reaching the floor.  center z = -0.175, half z = 0.425
    e, c, wb_c = make_box('DoorwayCutter', center=(0.0, 0.0, -0.175), half=(0.2, 0.6, 0.425))
    csg_subtract(wb, comp, e, wb_c, compose())   # cutter already positioned -> identity
    print('doorway demo: created WallJamb')


def demo_ledge():
    """
    A shelving-ledge profile: a deep notch cut from the top-front corner.

    Source:  unit cube (a block)
    Cutter:  unit cube offset so it overlaps only the top-right (x>0, z>0)
             quadrant of the source.
    Result:  an L-shaped cross section — the same profile used for
             coping stones, skirting boards, or stair nosings.

    Viewed from the side (x-z plane):
        ┌──┐
        │  │   ← back upright
        │  └───┐
        │      │  ← ledge shelf
        └──────┘
    """
    entity, comp, wb = make_cube('Ledge')
    # Oversize the cutter (scale 2 → half-extent 1.0) and place its -X and -Z
    # faces exactly on the cut planes x=0 and z=0, while its other faces
    # (x=2, z=2, y=±1) sit well OUTSIDE the source.  This is critical: a plain
    # unit cutter shares its y=±0.5 faces with the source, and coplanar faces
    # make the boolean produce garbage (stray diagonal/triangular cuts).  By
    # overshooting, the only cutter planes inside the source are the two we
    # actually want, giving a clean rectangular L-notch.
    transform = compose(translation=V3(1.0, 0.0, 1.0), scale=2.0)
    e, c, wb_c = make_cube('LedgeCutter')
    csg_subtract(wb, comp, e, wb_c, transform)
    print('ledge demo: created Ledge')


def demo_chamfer():
    """
    A 45-degree chamfered (bevelled) corner — impossible with extrude alone.

    Source:  unit cube
    Cutter:  unit cube rotated 45° around Z (the vertical axis) so it presents
             a 45° vertical face, placed so that face lies on the plane
             x - y = 0.7 and only shaves the front-right vertical edge.
    Result:  a block with one chamfered vertical edge (a 0.3 bevel).

    Viewed from above (x-y plane), corner at (+0.5, -0.5):
        ┌───────┐
        │       │
        │       ╲     ← 45° bevel on the front-right edge
        └──────╲
    """
    entity, comp, wb = make_cube('Chamfer')
    # Rotate the cutter -45° about Z (vertical), NOT Y - a vertical edge runs
    # along Z, so the cut plane must be vertical.  scale 1.2 (half-extent 0.6)
    # covers the full height in z and keeps the cutter's side faces outside the
    # corner.  Translating to (0.7745, -0.7745) puts the cutter's -X face on the
    # plane x - y = 0.7, which crosses the front face at x=0.2 and the right
    # face at y=-0.2 -> a clean 0.3 chamfer on the (x=+0.5, y=-0.5) edge.
    transform = compose(translation=V3(0.7745, -0.7745, 0.0),
                        scale=1.2,
                        rot_z=-math.pi / 4)
    e, c, wb_c = make_cube('ChamferCutter')
    csg_subtract(wb, comp, e, wb_c, transform)
    print('chamfer demo: created Chamfer')


# ── entry point ───────────────────────────────────────────────────────────────

MODES = {
    'doorway' : demo_doorway,
    'ledge'   : demo_ledge,
    'chamfer' : demo_chamfer,
}

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description='CSG subtract demo.')
    parser.add_argument('mode', nargs='?', default='ledge', choices=sorted(MODES))
    args = parser.parse_args()
    MODES[args.mode]()
