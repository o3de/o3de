"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import WhiteBoxMath as whiteBoxMath
import WhiteBoxInit as init 

import argparse
import azlmbr.legacy.general as general
import azlmbr.bus as bus
import azlmbr.whitebox.api as api

# usage: pyRunFile path/to/file/tetrahedron.py <radius>


# mathematically, a tetrahedron built with spherical coordinates will not be centered vertically at origin
# we need to calculate how far off it is to vertically center it
def calculate_offset_for_tetrahedron(radius):
    h1 = whiteBoxMath.spherical_to_cartesian(0.0, 0.0, radius).z
    h2 = whiteBoxMath.spherical_to_cartesian(109.5, 90.0, radius).z
    offset = (h1 + h2) * -0.5

    return whiteBoxMath.spherical_to_cartesian(0.0, 0.0, offset)


def create_tetrahedron(whiteBoxMesh, radius=0.75):
    # get coordinates for all the vertices using the internal angles of a tetrahedron
    offset = calculate_offset_for_tetrahedron(radius)
    pos1 = whiteBoxMath.spherical_to_cartesian(0.0, 0.0, radius).Add(offset)
    pos2 = whiteBoxMath.spherical_to_cartesian(109.5, 90.0, radius).Add(offset)
    pos3 = whiteBoxMath.spherical_to_cartesian(109.5, 210.0, radius).Add(offset)
    pos4 = whiteBoxMath.spherical_to_cartesian(109.5, 330.0, radius).Add(offset)

    # create vertices from all the coordinates
    v1 = whiteBoxMesh.AddVertex(pos1)
    v2 = whiteBoxMesh.AddVertex(pos2)
    v3 = whiteBoxMesh.AddVertex(pos3)
    v4 = whiteBoxMesh.AddVertex(pos4)

    # add polygons for each set of vertices
    fvh1 = whiteBoxMesh.AddTriPolygon(v1, v2, v3)
    fvh2 = whiteBoxMesh.AddTriPolygon(v1, v3, v4)
    fvh3 = whiteBoxMesh.AddTriPolygon(v1, v4, v2)
    fvh4 = whiteBoxMesh.AddTriPolygon(v2, v4, v3)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Creates a tetrahedron.')
    parser.add_argument('radius', nargs='?', default=0.75, type=float, help='radius of the tetrahedron')
    args = parser.parse_args()

    # initialize whiteBoxMesh
    whiteBoxEntity = init.create_white_box_entity("WhiteBox-Tetrahedron")
    whiteBoxMeshComponent = init.create_white_box_component(whiteBoxEntity)
    whiteBoxMesh = init.create_white_box_handle(whiteBoxMeshComponent)

    # clear whiteBoxMesh to make a tetrahedron from scratch
    whiteBoxMesh.Clear()

    create_tetrahedron(whiteBoxMesh, args.radius)

    # update whiteBoxMesh
    init.update_white_box(whiteBoxMesh, whiteBoxMeshComponent)