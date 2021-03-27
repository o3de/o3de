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

# usage: pyRunFile path/to/file/cylinder.py <sides> <size>
# number of sides is used to determine the number of vertices in each circle


def create_cylinder(whiteBoxMesh, sides=16, size=0.5):
    # create all the vertices as two circles (top/bottom) with one additional vertex for the center of each circle
    # create center vertices for each circle
    top_vertex_pos = whiteBoxMath.spherical_to_cartesian(0.0, 0.0, size)
    bottom_vertex_pos = whiteBoxMath.spherical_to_cartesian(180.0, 0.0, size)
    top_vertex = whiteBoxMesh.AddVertex(top_vertex_pos)
    bottom_vertex = whiteBoxMesh.AddVertex(bottom_vertex_pos)

    # set up angles/distance for each of the cylinder circles
    top_circle_vertices = []
    bottom_circle_vertices = []
    angle_increment = 360.0 / sides

    # distance from the center of the cylinder to the vertices of the top/bottom circles
    # simplifies to size times the square root of 2
    vertex_dist = size * 1.414
    # add vertices for each of the cylinder circles
    for side in range (0, sides):
        # internal angle from center of cylinder to any point on top/bottom circles
        top_circle_pos = whiteBoxMath.spherical_to_cartesian(45.0, side * angle_increment, vertex_dist)
        bottom_circle_pos = whiteBoxMath.spherical_to_cartesian(135.0, side * angle_increment, vertex_dist)

        # add to list for vertices for the top/bottom circles
        top_circle_vertex = whiteBoxMesh.AddVertex(top_circle_pos)
        bottom_circle_vertex = whiteBoxMesh.AddVertex(bottom_circle_pos)

        top_circle_vertices.append(top_circle_vertex)
        bottom_circle_vertices.append(bottom_circle_vertex)

    # create faces
    top_circle_fvh = []
    bottom_circle_fvh = []
    for side in range (0, sides):
       index1 = side
       index2 = (side + 1) % sides

       # add to list for face vertex handles for top/bottom circles
       top_circle_fvh.append(api.util_MakeFaceVertHandles(top_vertex, top_circle_vertices[index1], top_circle_vertices[index2]))
       bottom_circle_fvh.append(api.util_MakeFaceVertHandles(bottom_vertex, bottom_circle_vertices[index2], bottom_circle_vertices[index1]))

       # add quad polygons to create the side of the cylinder
       whiteBoxMesh.AddQuadPolygon(top_circle_vertices[index1], bottom_circle_vertices[index1], bottom_circle_vertices[index2], top_circle_vertices[index2])

    # add top/bottom faces
    whiteBoxMesh.AddPolygon(top_circle_fvh)
    whiteBoxMesh.AddPolygon(bottom_circle_fvh)


if __name__ == "__main__":
    # cmdline arguments
    parser = argparse.ArgumentParser(description='Creates a cylinder shaped white box mesh.')
    parser.add_argument('sides', nargs='?', default=16, type=int, help='number of vertices in each circle')
    parser.add_argument('size', nargs='?', default=0.5, type=float, help='size of the cylinder')
    args = parser.parse_args()

    # initialize whiteBoxMesh
    whiteBoxEntity = init.create_white_box_entity("WhiteBox-Cylinder")
    whiteBoxMeshComponent = init.create_white_box_component(whiteBoxEntity)
    whiteBoxMesh = init.create_white_box_handle(whiteBoxMeshComponent)

    # clear whiteBoxMesh to make a cylinder from scratch
    whiteBoxMesh.Clear()
    
    create_cylinder(whiteBoxMesh, args.sides, args.size)

    # update whiteBoxMesh
    init.update_white_box(whiteBoxMesh, whiteBoxMeshComponent)