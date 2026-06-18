"""
Diagnostic: build the chamfer boolean (reusing CsgDemo's fixed helpers) and
report the resulting mesh's vertex/face counts.

A clean 45-degree chamfer of one vertical edge on a unit cube is watertight
and should come out as roughly 10 vertices / 16 triangles.

  - If the FACE count is short (e.g. ~14), triangles were dropped by OpenMesh's
    add_face during RebuildFromTriangleMesh -> a HOLE in the cap (you see the
    dark/blue inside of the box through it).
  - If the counts are ~10 / ~16 and watertight, the cap exists but a normal is
    flipped -> a winding issue instead.

    pyRunFile path/to/CsgInspect.py
"""

import math
import importlib

import azlmbr.bus as bus
import azlmbr.math
import WhiteBoxInit as init

# reload the demo so we pick up the latest helpers/fixes
import CsgDemo
importlib.reload(CsgDemo)

V3 = azlmbr.math.Vector3


def counts(handle):
    return handle.MeshVertexCount(), handle.MeshFaceCount()


entity, comp, wb     = CsgDemo.make_cube('Chamfer-Inspect')
e_cut, c_cut, wb_cut = CsgDemo.make_cube('Chamfer-Inspect-Cutter')

print('--- source cube ---')
print('  verts/faces:', counts(wb), '(expect 8 / 12)')

transform = CsgDemo.compose(translation=V3(0.7745, -0.7745, 0.0), scale=1.2, rot_z=-math.pi / 4)
ok = wb.MeshBoolean(wb_cut, transform, 1)   # 1 == SUBTRACTION
print('--- boolean returned:', ok, '---')

if ok:
    print('  result verts/faces:', counts(wb))
    print('  (clean chamfer expected: ~10 verts / ~16 faces, watertight)')
    init.update_white_box(wb, comp)
    import azlmbr.editor as editor
    editor.ToolsApplicationRequestBus(bus.Broadcast, 'DeleteEntityById', e_cut)
else:
    print('  boolean failed - check warnings above')
