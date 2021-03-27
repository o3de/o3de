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

# usage: pyRunFile path/to/file/staircase.py <steps> <depth> <height> <width>

# formula to determine the new amount of faces added with each polygon translation
def faces_added(step):
    return (step + 2) * 4


# returns the face handles used to make the top back block, used for extruding upward
def top_back_face_handle(num_faces):
    return api.FaceHandle(num_faces - 4)


# determines the edge that needs to be hidden at the back of the staircase
# next edge seems to related to prev edge by the recursive sequence e(n+1) = e(n)+20+6*n
def edge_to_hide(prev_edge, step):
    if step == 0:
        return 20
    return prev_edge + 20 + 6 * step
    

def create_staircase_from_white_box_mesh(whiteBoxMesh, num_steps, depth, height, width):
    # number of faces the mesh starts with
    curr_faces = 12
    # backmost face of the white box mesh, this will be extruded to add new steps for the staircase
    back_face = api.FaceHandle(10)
    # backmost face of the white box mesh, this will be extruded to add width to the staircase
    side_face = api.FaceHandle(5)

    # change the default whiteBoxMesh which will act as the first step to the staircase
    if height != 1.0 and height > 0.0:
        offset = height - 1.0
        top_polygon = whiteBoxMesh.FacePolygonHandle(azlmbr.object.construct('FaceHandle', 0))
        whiteBoxMesh.TranslatePolygon(top_polygon, offset)
    if depth != 1.0 and depth > 0.0:
        offset = depth - 1.0
        back_polygon = whiteBoxMesh.FacePolygonHandle(back_face)
        whiteBoxMesh.TranslatePolygon(back_polygon, offset)
    if width != 1.0 and width > 0.0:
        offset = width - 1.0
        side_polygon = whiteBoxMesh.FacePolygonHandle(side_face)
        whiteBoxMesh.TranslatePolygon(side_polygon, offset)
    

    prev_edge = 0
    # create rest of the staircase steps
    for step in range(0, num_steps):
        # extrude the back to create room for a new step
        back_polygon = whiteBoxMesh.TranslatePolygonAppend(whiteBoxMesh.FacePolygonHandle(back_face), depth)
        curr_faces += faces_added(step)
        back_faces = back_polygon.FaceHandles
        back_face = back_faces[-1]
        # extrude upward to create the new step
        top_back_polygon = whiteBoxMesh.FacePolygonHandle(top_back_face_handle(curr_faces))
        whiteBoxMesh.TranslatePolygonAppend(top_back_polygon, height)
        curr_faces += 8
        # hide any edge created in the backside from extruding upward
        prev_edge = edge_to_hide(prev_edge, step)
        whiteBoxMesh.HideEdge(api.EdgeHandle(prev_edge))


def create_staircase(whiteBoxMesh, num_steps=2, depth=1.0, height=1.0, width=1.0):
    # if calling create_staircase directly, we need to start with a unit cube
    whiteBoxMesh.InitializeAsUnitCube()
    create_staircase_from_white_box_mesh(whiteBoxMesh, num_steps, depth, height, width)

    
if __name__ == "__main__":
    # cmdline arguments
    parser = argparse.ArgumentParser(description='Creates a staircase shaped white box mesh.')
    parser.add_argument('num_steps', nargs='?', default=4, type=int, help='number of steps in the staircase')
    parser.add_argument('depth', nargs='?', default=1.0, type=float, help='depth of each step in the staircase')
    parser.add_argument('height', nargs='?', default=1.0, type=float, help='height of each step in the staircase')
    parser.add_argument('width', nargs='?', default=1.0, type=float, help='width of each step in the staircase')
    args = parser.parse_args()

    # initialize whiteBoxMesh
    whiteBoxEntity = init.create_white_box_entity("WhiteBox-Staircase")
    whiteBoxMeshComponent = init.create_white_box_component(whiteBoxEntity)
    whiteBoxMesh = init.create_white_box_handle(whiteBoxMeshComponent)

    create_staircase_from_white_box_mesh(whiteBoxMesh, args.num_steps, args.depth, args.height, args.width)

    # update whiteBoxMesh
    init.update_white_box(whiteBoxMesh, whiteBoxMeshComponent)