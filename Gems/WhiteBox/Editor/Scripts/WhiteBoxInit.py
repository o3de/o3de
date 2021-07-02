"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# setup path
import azlmbr.legacy.general as general
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity
import azlmbr.object
import azlmbr.math
import azlmbr.whitebox.api as api
from azlmbr.entity import EntityId
from azlmbr.entity import EntityType


# get Component Type for WhiteBoxMesh
def get_white_box_component_type():
    typeIdsList = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', ['White Box'], EntityType().Game)
    whiteBoxMeshComponentTypeId = typeIdsList[0]

    return whiteBoxMeshComponentTypeId


# use old White Box entity to hold White Box component if it exists, otherwise use a new one
def create_white_box_entity(name="WhiteBox"):
    newEntityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', EntityId())

    editor.EditorEntityAPIBus(bus.Event, 'SetName', newEntityId, name)

    return newEntityId


# add whiteBoxMeshComponent to entity and enable
def create_white_box_component(entityId):
    whiteBoxMeshComponentTypeId = get_white_box_component_type()

    whiteBoxMeshComponentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', entityId, [whiteBoxMeshComponentTypeId])
    whiteBoxMeshComponents = whiteBoxMeshComponentOutcome.GetValue()
    whiteBoxMeshComponent = whiteBoxMeshComponents[0]
    editor.EditorComponentAPIBus(bus.Broadcast, 'EnableComponents', whiteBoxMeshComponents)

    return whiteBoxMeshComponent


# create whiteBoxMeshHandle from bus call
def create_white_box_handle(whiteBoxMeshComponent):
    whiteBoxMesh = azlmbr.whitebox.request.bus.EditorWhiteBoxComponentRequestBus(bus.Event, 'GetWhiteBoxMeshHandle', whiteBoxMeshComponent)

    return whiteBoxMesh


# update normals, uvs, and notify white box mesh
def update_white_box(whiteBoxMesh, whiteBoxMeshComponent):
    whiteBoxMesh.CalculateNormals()
    whiteBoxMesh.CalculatePlanarUVs()
    azlmbr.whitebox.notification.bus.EditorWhiteBoxComponentNotificationBus(bus.Event, 'OnWhiteBoxMeshModified', whiteBoxMeshComponent)
    azlmbr.whitebox.request.bus.EditorWhiteBoxComponentRequestBus(bus.Event, 'SerializeWhiteBox', whiteBoxMeshComponent)
