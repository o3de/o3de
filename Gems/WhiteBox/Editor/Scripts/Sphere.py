"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import Icosahedron
import WhiteBoxMath as whiteBoxMath
import WhiteBoxInit as init 

import argparse
import azlmbr.legacy.general as general
import azlmbr.bus as bus
import azlmbr.whitebox.api as api

# usage: pyRunFile path/to/file/sphere.py <subdivisions>


# get the midpoint of v1 and v2 from created_midpoints if already made, otherwise create new one
def get_midpoint_vertex(whiteBoxMesh, v1, v2, radius, created_midpoints):
    # get the index of the vertices to look them up
    index1 = v1.Index()
    index2 = v2.Index()

    # search created_midpoints to see if this midpoint has already been made
    # we store edges as tuples but keep in mind the edge (v1, v2) == (v2, v1) so we search both
    if (index1, index2) in created_midpoints:
        return created_midpoints.get((index1, index2))
    if (index2, index1) in created_midpoints:
        return created_midpoints.get((index2, index1))

    # create the new midpoint vertex and store in created_midpoints
    pos1 = whiteBoxMesh.VertexPosition(v1)
    pos2 = whiteBoxMesh.VertexPosition(v2)
    midpoint = whiteBoxMesh.AddVertex(whiteBoxMath.normalize_midpoint(pos1, pos2, radius))
    created_midpoints.update({(index1, index2): midpoint})

    return midpoint


# divide each triangular face into four smaller faces
def subdivide_faces(whiteBoxMesh, faces, radius):
    new_faces = []
    created_midpoints = dict()
    for faceVertHandles in faces:
        # get each vertex
        v0 = faceVertHandles.VertexHandles[0]
        v1 = faceVertHandles.VertexHandles[1]
        v2 = faceVertHandles.VertexHandles[2]

        # get the vertex representing the midpoint of each of the edges
        v3 = get_midpoint_vertex(whiteBoxMesh, v0, v1, radius, created_midpoints)
        v4 = get_midpoint_vertex(whiteBoxMesh, v1, v2, radius, created_midpoints)
        v5 = get_midpoint_vertex(whiteBoxMesh, v0, v2, radius, created_midpoints)

        # create four subdivided faces for each original face
        new_faces.append(api.util_MakeFaceVertHandles(v0, v3, v5))
        new_faces.append(api.util_MakeFaceVertHandles(v3, v1, v4))
        new_faces.append(api.util_MakeFaceVertHandles(v4, v2, v5))
        new_faces.append(api.util_MakeFaceVertHandles(v3, v4, v5))

    return new_faces
    

# create sphere by subdividing an icosahedron
def create_sphere(whiteBoxMesh, subdivisions=2, radius=0.55):
    # create icosahedron faces and subdivide them to create a sphere
    icosahedron_faces = Icosahedron.create_icosahedron_faces(whiteBoxMesh, radius)
    for division in range (0, subdivisions):
        icosahedron_faces = subdivide_faces(whiteBoxMesh, icosahedron_faces, radius)
    for face in icosahedron_faces:
        whiteBoxMesh.AddPolygon([face])


if __name__ == "__main__":
    # cmdline arguments
    parser = argparse.ArgumentParser(description='Creates a sphere shaped white box mesh.')
    parser.add_argument('subdivisions', nargs='?', default=3, choices=range(0, 5), type=int, help='number of subdivisions to form sphere from icosahedron')
    parser.add_argument('radius', nargs='?', default=0.55, type=float, help='radius of the sphere')
    args = parser.parse_args()

    # initialize whiteBoxMesh
    whiteBoxEntity = init.create_white_box_entity("WhiteBox-Sphere")
    whiteBoxMeshComponent = init.create_white_box_component(whiteBoxEntity)
    whiteBoxMesh = init.create_white_box_handle(whiteBoxMeshComponent)

    # clear whiteBoxMesh to make a sphere from scratch
    whiteBoxMesh.Clear()

    create_sphere(whiteBoxMesh, args.subdivisions, args.radius)

    # update whiteBoxMesh
    init.update_white_box(whiteBoxMesh, whiteBoxMeshComponent)
