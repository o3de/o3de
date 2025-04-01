"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import Cylinder
import Icosahedron
import Sphere
import Staircase
import Tetrahedron
import WhiteBoxInit as init

import sys
import azlmbr.legacy.general as general
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity
import azlmbr.object
import azlmbr.math
import azlmbr.whitebox.api as api


def on_change_default_shape_type(whiteBoxMeshComponent, defaultShapeType):
    # return if defaultShapeType does not need python execution or is invalid
    if not 0 <= defaultShapeType <= 4:
        return

    # define switch for shape functions
    switch={
        1: Tetrahedron.create_tetrahedron,
        2: Icosahedron.create_icosahedron,
        3: Cylinder.create_cylinder,
        4: Sphere.create_sphere
    }

    # get the white box mesh handle
    whiteBoxMesh = azlmbr.whitebox.request.bus.EditorWhiteBoxComponentRequestBus(bus.Event, 'GetWhiteBoxMeshHandle', whiteBoxMeshComponent)
    
    if not whiteBoxMesh.IsValid():
        print('Could not resolve White Box Mesh')
        return

    # clear whiteBoxMesh
    whiteBoxMesh.Clear()

    # if defaultShapeType is 0, initialize as cube
    if (defaultShapeType == 0):
        whiteBoxMesh.InitializeAsUnitCube()
    # else find the correct shape creation function and call it
    else:
        shape_creation_func = switch.get(defaultShapeType)
        shape_creation_func(whiteBoxMesh)

    # update white box mesh
    whiteBoxMesh.CalculateNormals()
    whiteBoxMesh.CalculatePlanarUVs()
    azlmbr.whitebox.request.bus.EditorWhiteBoxComponentModeRequestBus(bus.Event, 'MarkWhiteBoxIntersectionDataDirty', whiteBoxMeshComponent)
    azlmbr.whitebox.notification.bus.EditorWhiteBoxComponentNotificationBus(bus.Event, 'OnWhiteBoxMeshModified', whiteBoxMeshComponent)
    azlmbr.whitebox.request.bus.EditorWhiteBoxComponentRequestBus(bus.Event, 'SerializeWhiteBox', whiteBoxMeshComponent)


if __name__ == "__main__":
    entityId = int(sys.argv[1])
    componentId =  int(sys.argv[2])
    entityComponentIdPair = api.util_MakeEntityComponentIdPair(entityId, componentId)
    shapeType = int(sys.argv[3])
    on_change_default_shape_type(entityComponentIdPair, shapeType)
