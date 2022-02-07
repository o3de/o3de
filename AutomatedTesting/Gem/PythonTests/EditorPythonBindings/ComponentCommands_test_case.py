"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# It needs a new test level in prefab format to make it testable again.

# Tests a portion of the Component CRUD Python API while the Editor is running

import azlmbr.bus as bus
import azlmbr.entity as entity
import azlmbr.editor as editor
import azlmbr.object
import azlmbr.math
from azlmbr.entity import EntityId

def CompareComponentEntityIdPairs(component1, component2):
    return component1.Equal(component2)

# Open a level (any level should work)
editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'OpenLevelNoPrompt', 'Base')

# Get Component Types for Mesh and Comment
typeIdsList = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', ["Mesh", "Comment", "Mesh Collider"], entity.EntityType().Game)

if(len(typeIdsList) > 0):
    print("Type Ids List returned correctly")

meshComponentTypeId = typeIdsList[0]
commentComponentTypeId = typeIdsList[1]
meshColliderComponentTypeId = typeIdsList[2]

# Get Component Ids from Component Types
typeNamesList = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeNames', typeIdsList)

if(typeNamesList[0] == "Mesh") and (typeNamesList[1] == "Comment") and (typeNamesList[2] == "Mesh Collider"):
    print("Type Names List returned correctly")

# Test Component API
newEntityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', EntityId())

if (newEntityId):
    print("New entity with no parent created")

hadComponent = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', newEntityId, meshComponentTypeId)

if not(hadComponent):
    print("Entity does not have a Mesh component")

meshComponentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', newEntityId, [meshComponentTypeId])

if (meshComponentOutcome.IsSuccess()):
    print("Mesh component added to entity")

meshComponents = meshComponentOutcome.GetValue()
meshComponent = meshComponents[0]

if(meshComponent.get_entity_id() == newEntityId):
    print("EntityId on the meshComponent EntityComponentIdPair matches")

if not(meshComponent.to_string() == ""):
    print("EntityComponentIdPair to_string works")

hasComponent = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', newEntityId, meshComponentTypeId)

if(hasComponent):
    print("Entity has a Mesh component")

isActive = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', meshComponent)

if(isActive):
    print("Mesh component is active")

editor.EditorComponentAPIBus(bus.Broadcast, 'DisableComponents', [meshComponent])

isNotActive = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', meshComponent)

if not(isNotActive):
    print("Mesh component is not active")

if(editor.EditorComponentAPIBus(bus.Broadcast, 'IsValid', meshComponent)):
    print("Mesh component is valid")

CommentComponentsOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', newEntityId, [commentComponentTypeId, commentComponentTypeId])

if (CommentComponentsOutcome.IsSuccess()):
    print("Comment components added to entity")

CommentComponents = CommentComponentsOutcome.GetValue()

GetCommentComponentsOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentsOfType', newEntityId, commentComponentTypeId)

if(GetCommentComponentsOutcome.IsSuccess()):
    GetCommentComponents = GetCommentComponentsOutcome.GetValue()

if(CompareComponentEntityIdPairs(CommentComponents[0], GetCommentComponents[0]) and CompareComponentEntityIdPairs(CommentComponents[1], GetCommentComponents[1])):
    print("Got both Comment components")

if(CompareComponentEntityIdPairs(CommentComponents[0], GetCommentComponents[1]) and CompareComponentEntityIdPairs(CommentComponents[1], GetCommentComponents[0])):
    print("Got both Comment components")

GetComponentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', newEntityId, meshComponentTypeId)

if(GetComponentOutcome.IsSuccess() and CompareComponentEntityIdPairs(GetComponentOutcome.GetValue(), meshComponent)):
    print("GetComponent works")

commentsCount = editor.EditorComponentAPIBus(bus.Broadcast, 'CountComponentsOfType', newEntityId, commentComponentTypeId)

if(commentsCount == 2):
    print("Entity has two Comment components")

editor.EditorComponentAPIBus(bus.Broadcast, 'DisableComponents', CommentComponents)
isCActive = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', CommentComponents[0])
isC2Active = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', CommentComponents[1])

if not(isCActive) and not(isC2Active):
    print("Disabled both Comment components")

editor.EditorComponentAPIBus(bus.Broadcast, 'EnableComponents', CommentComponents)
isCActive = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', CommentComponents[0])
isC2Active = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', CommentComponents[1])

if (isCActive) and (isC2Active):
    print("Enabled both Comment components")

editor.EditorComponentAPIBus(bus.Broadcast, 'RemoveComponents', [meshComponent])

hasMesh = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', newEntityId, meshComponentTypeId)

componentSingleOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentOfType', newEntityId, commentComponentTypeId)

if (componentSingleOutcome.IsSuccess()):
    print("Single comment component added to entity")

commentsCount = editor.EditorComponentAPIBus(bus.Broadcast, 'CountComponentsOfType', newEntityId, commentComponentTypeId)

if (commentsCount == 3):
    print("Entity has three Comment components")

if not(hasMesh):
    print("Mesh Component removed")

if not(editor.EditorComponentAPIBus(bus.Broadcast, 'IsValid', meshComponent)):
    print("Mesh component is no longer valid")


# Test that it is possible to access Components with no Editor Component (for example, the legacy mesh collider)

meshColliderComponentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', newEntityId, [meshColliderComponentTypeId])

if (meshColliderComponentOutcome.IsSuccess()):
    print("Mesh Collider component added to entity")

meshColliderComponent = meshColliderComponentOutcome.GetValue()

getMeshColliderComponentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', newEntityId, meshComponentTypeId)

if(getMeshColliderComponentOutcome.IsSuccess() and CompareComponentEntityIdPairs(meshColliderComponent, getMeshColliderComponentOutcome.GetValue())):
    print("Mesh Collider component retrieved from entity")

editor.EditorComponentAPIBus(bus.Broadcast, 'RemoveComponents', [meshColliderComponent])

hasMeshCollider = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', newEntityId, meshColliderComponentTypeId)

if not(hasMeshCollider):
    print("Mesh Collider Component removed")


# Test that it is possible to access Components with no Editor Component(for example, the legacy mesh collider) via GetComponentOfType

meshColliderComponentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', newEntityId, [meshColliderComponentTypeId])

if (meshColliderComponentOutcome.IsSuccess()):
    print("Mesh Collider component added to entity")

meshColliderComponent = meshColliderComponentOutcome.GetValue()[0]

getMeshColliderComponentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', newEntityId, meshColliderComponentTypeId)

if(getMeshColliderComponentOutcome.IsSuccess() and CompareComponentEntityIdPairs(meshColliderComponent, getMeshColliderComponentOutcome.GetValue())):
    print("Mesh Collider component retrieved from entity")

meshColliderRemoved = False;
meshColliderRemoved = editor.EditorComponentAPIBus(bus.Broadcast, 'RemoveComponents', [meshColliderComponent])

if meshColliderRemoved:
    print("Mesh Collider component removed from entity")

editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'ExitNoPrompt')
