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

import azlmbr.legacy.general as general
import azlmbr.bus as bus
import azlmbr.whitebox.api as api

# usage: pyRunFile path/to/file/icosahedron.py <radius>


# create the faces which will be used in the icosahedron
def create_icosahedron_faces(whiteBoxMesh, radius):
    # get coordinates for all the vertices using the internal angles of a icosahedron
    # upper side
    pos1 = whiteBoxMath.spherical_to_cartesian(0.0, 0.0, radius)
    pos2 = whiteBoxMath.spherical_to_cartesian(63.43, 18.0, radius)
    pos3 = whiteBoxMath.spherical_to_cartesian(63.43, 90.0, radius)
    pos4 = whiteBoxMath.spherical_to_cartesian(63.43, 162.0, radius)
    pos5 = whiteBoxMath.spherical_to_cartesian(63.43, 234.0, radius)
    pos6 = whiteBoxMath.spherical_to_cartesian(63.43, 306.0, radius)

    # lower side
    pos7 = whiteBoxMath.spherical_to_cartesian(180.0, 0.0, radius)
    pos8 = whiteBoxMath.spherical_to_cartesian(116.57, 52.0, radius)
    pos9 = whiteBoxMath.spherical_to_cartesian(116.57, 126.0, radius)
    pos10 = whiteBoxMath.spherical_to_cartesian(116.57, 198.0, radius)
    pos11 = whiteBoxMath.spherical_to_cartesian(116.57, 270.0, radius)
    pos12 = whiteBoxMath.spherical_to_cartesian(116.57, 342.0, radius)

    # create vertices from all the coordinates
    # upper side
    v1 = whiteBoxMesh.AddVertex(pos1)
    v2 = whiteBoxMesh.AddVertex(pos2)
    v3 = whiteBoxMesh.AddVertex(pos3)
    v4 = whiteBoxMesh.AddVertex(pos4)
    v5 = whiteBoxMesh.AddVertex(pos5)
    v6 = whiteBoxMesh.AddVertex(pos6)

    # lower side
    v7 = whiteBoxMesh.AddVertex(pos7)
    v8 = whiteBoxMesh.AddVertex(pos8)
    v9 = whiteBoxMesh.AddVertex(pos9)
    v10 = whiteBoxMesh.AddVertex(pos10)
    v11 = whiteBoxMesh.AddVertex(pos11)
    v12 = whiteBoxMesh.AddVertex(pos12)

    # add faces to list
    faces = []
    # upper side
    fvh1 = faces.append(api.util_MakeFaceVertHandles(v1, v2, v3))
    fvh2 = faces.append(api.util_MakeFaceVertHandles(v1, v3, v4))
    fvh3 = faces.append(api.util_MakeFaceVertHandles(v1, v4, v5))
    fvh4 = faces.append(api.util_MakeFaceVertHandles(v1, v5, v6))
    fvh5 = faces.append(api.util_MakeFaceVertHandles(v1, v6, v2))

    # lower side
    fvh6 = faces.append(api.util_MakeFaceVertHandles(v7, v12, v11))
    fvh7 = faces.append(api.util_MakeFaceVertHandles(v7, v11, v10))
    fvh8 = faces.append(api.util_MakeFaceVertHandles(v7, v10, v9))
    fvh9 = faces.append(api.util_MakeFaceVertHandles(v7, v9, v8))
    fvh10 = faces.append(api.util_MakeFaceVertHandles(v7, v8, v12))

    # middle side
    fvh11 = faces.append(api.util_MakeFaceVertHandles(v12, v8, v2))
    fvh12 = faces.append(api.util_MakeFaceVertHandles(v8, v9, v3))
    fvh13 = faces.append(api.util_MakeFaceVertHandles(v9, v10, v4))
    fvh14 = faces.append(api.util_MakeFaceVertHandles(v10, v11, v5))
    fvh15 = faces.append(api.util_MakeFaceVertHandles(v11, v12, v6))

    fvh16 = faces.append(api.util_MakeFaceVertHandles(v2, v8, v3))
    fvh17 = faces.append(api.util_MakeFaceVertHandles(v3, v9, v4))
    fvh18 = faces.append(api.util_MakeFaceVertHandles(v4, v10, v5))
    fvh19 = faces.append(api.util_MakeFaceVertHandles(v5, v11, v6))
    fvh20 = faces.append(api.util_MakeFaceVertHandles(v6, v12, v2))

    return faces


def create_icosahedron(whiteBoxMesh, radius=0.6):
    # create list of faces to add to polygon
    icosahedron_faces = create_icosahedron_faces(whiteBoxMesh, radius)

    # add polygons to white box mesh
    for face in icosahedron_faces:
        whiteBoxMesh.AddPolygon([face])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Creates an icosahedron.')
    parser.add_argument('radius', nargs='?', default=0.6, type=int, help='radius of the icosahedron')
    args = parser.parse_args()

    whiteBoxEntity = init.create_white_box_entity("WhiteBox-Icosahedron")
    whiteBoxMeshComponent = init.create_white_box_component(whiteBoxEntity)
    whiteBoxMesh = init.create_white_box_handle(whiteBoxMeshComponent)

    # clear whiteBoxMesh to make a icosahedron from scratch
    whiteBoxMesh.Clear()
    create_icosahedron(whiteBoxMesh, args.radius)

    # update whiteBoxMesh
    init.update_white_box(whiteBoxMesh, whiteBoxMeshComponent)