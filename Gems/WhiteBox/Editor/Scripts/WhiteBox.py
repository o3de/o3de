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

# get Component Type for WhiteBoxMesh
whiteBoxMeshComponentTypeId = get_white_box_component_type()

# use old White Box entity to hold White Box component if it exists, otherwise use a new one
newEntityId = None
oldEntityId = general.find_editor_entity('WhiteBox')

if oldEntityId.IsValid():
    whiteBoxMeshComponentExists = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', oldEntityId, whiteBoxMeshComponentTypeId)
    if (whiteBoxMeshComponentExists):
        oldwhiteBoxMeshComponent = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', oldEntityId, whiteBoxMeshComponentTypeId)
        editor.EditorComponentAPIBus(bus.Broadcast, 'RemoveComponents', [oldwhiteBoxMeshComponent.GetValue()])
        newEntityId = oldEntityId
else:
    newEntityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', EntityId())
    editor.EditorEntityAPIBus(bus.Event, 'SetName', newEntityId, "WhiteBox")

# add whiteBoxMeshComponent to entity and enable
whiteBoxMeshComponentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', newEntityId, [whiteBoxMeshComponentTypeId])

if (whiteBoxMeshComponentOutcome.IsSuccess()):
    print("White Box Component added to entity.")

whiteBoxMeshComponents = whiteBoxMeshComponentOutcome.GetValue()
whiteBoxMeshComponent = whiteBoxMeshComponents[0]

editor.EditorComponentAPIBus(bus.Broadcast, 'EnableComponents', whiteBoxMeshComponents)

isComponentEnabled = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', whiteBoxMeshComponent)
if (isComponentEnabled):
    print("Enabled Mesh component.")

whiteBoxMesh = azlmbr.whitebox.request.bus.EditorWhiteBoxComponentRequestBus(bus.Event, 'GetWhiteBoxMeshHandle', whiteBoxMeshComponent)

# translate append (extrude) a polygon
if (len(sys.argv) >= 2 and float(sys.argv[2]) != 0.0):
    # create face handle from user input (argv[1])
    faceHandle = azlmbr.object.construct('FaceHandle', int(sys.argv[1]))
    # find the polygon handle that corresponds to the given face
    facePolygonHandle = whiteBoxMesh.FacePolygonHandle(faceHandle)
    # translate append (extrude) the polygon by a distance specified by the user (argv[2])
    whiteBoxMesh.TranslatePolygonAppend(facePolygonHandle, float(sys.argv[2]))
    # recalculate uvs as mesh will have changed
    whiteBoxMesh.CalculatePlanarUVs()
    # notify the white box component the mesh has changed to force it to rebuild the render mesh
    azlmbr.whitebox.notification.bus.EditorWhiteBoxComponentNotificationBus(bus.Event, 'OnWhiteBoxMeshModified', whiteBoxMeshComponent)
