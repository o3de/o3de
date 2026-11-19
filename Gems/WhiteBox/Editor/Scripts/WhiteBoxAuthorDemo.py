"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Demonstrates authoring White Box meshes from code with whitebox_author.

    pyRunFile path/to/WhiteBoxAuthorDemo.py

Builds a stepped tower (inset + extrude), a pyramid from a raw triangle list and a pillar with a carved slot, side by side.
The whole script is one undo step.
"""

import whitebox_author as wb

with wb.undo("White Box authoring demo"):
    # a parametric shape, then repeated inset + extrude on the top polygon to step it upwards
    tower = wb.WhiteBox.create("DemoTower", position=(0, 0, 0), shape="box", width=2, depth=2, height=1)
    top = tower.facing((0, 0, 1))
    for _ in range(3):
        top = tower.inset(top, 0.2)
        top = tower.extrude(top, 1.0)
    tower.set_layer(name="Tower", tint=(0.8, 0.7, 0.5))

    # a mesh from raw positions and counter-clockwise triangles (a square pyramid)
    pyramid = wb.WhiteBox.create("DemoPyramid", position=(4, 0, 0))
    pyramid.add_layer()
    pyramid.set_mesh(
        [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0), (0.5, 0.5, 1)],
        [(0, 2, 1), (0, 3, 2), (0, 1, 4), (1, 2, 4), (2, 3, 4), (3, 0, 4)])

    # a pillar with a slot cut out through its layers: a second layer subtracts from the one below
    pillar = wb.WhiteBox.create("DemoPillar", position=(8, 0, 0), shape="cylinder", width=1, depth=1, height=3, sides=16)
    pillar.add_shape("box", width=0.4, depth=2, height=1)
    pillar.set_layer(name="Slot", combine_mode="subtract", position=(0, 0, 1.0))

for box in (tower, pyramid, pillar):
    info = box.describe()
    print(info["layers"][0]["name"], "faces:", info["evaluated"].get("faces"), "bounds:", info["evaluated"].get("bounds"))
