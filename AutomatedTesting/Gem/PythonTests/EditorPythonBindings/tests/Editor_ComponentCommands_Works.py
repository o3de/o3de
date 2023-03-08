"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os, sys
sys.path.append(os.path.dirname(__file__))
from Editor_TestClass import BaseClass
from EditorPythonTestTools.consts.physics import PHYSX_MESH_COLLIDER

class Editor_ComponentCommands_Works(BaseClass):
    # Description: 
    # Tests a portion of the Component CRUD Python API while the Editor is running
    
    @staticmethod
    def test():
        import azlmbr.bus as bus
        import azlmbr.entity as entity
        import azlmbr.editor as editor
        import azlmbr.object
        import azlmbr.math
        from azlmbr.entity import EntityId

        def CompareComponentEntityIdPairs(component1, component2):
            return component1.Equal(component2)

        # Get Component Types for Mesh and Comment
        typeNameList = ["Mesh", "Comment", PHYSX_MESH_COLLIDER]
        typeIdsList = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', typeNameList, entity.EntityType().Game)

        BaseClass.check_result(len(typeIdsList) > 0, "Type Ids List returned correctly")

        meshComponentTypeId = typeIdsList[0]
        commentComponentTypeId = typeIdsList[1]
        meshColliderComponentTypeId = typeIdsList[2]

        # Get Component Ids from Component Types
        typeNamesList = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeNames', typeIdsList)
        BaseClass.check_result(typeNamesList[0] == "Mesh", "Type Names List contains: Mesh")
        BaseClass.check_result(typeNamesList[1] == "Comment", "Type Names List contains: Comment")
        BaseClass.check_result(typeNamesList[2] == PHYSX_MESH_COLLIDER, "Type Names List contains: PhysX Mesh Collider")

        # Test Component API
        newEntityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', EntityId())

        BaseClass.check_result(newEntityId,  "New entity with no parent created")

        hadComponent = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', newEntityId, meshComponentTypeId)

        BaseClass.check_result(not(hadComponent), "Entity does not have a Mesh component")

        meshComponentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', newEntityId, [meshComponentTypeId])

        BaseClass.check_result(meshComponentOutcome.IsSuccess(), "Mesh component added to entity")

        meshComponents = meshComponentOutcome.GetValue()
        meshComponent = meshComponents[0]

        BaseClass.check_result(meshComponent.get_entity_id() == newEntityId, "EntityId on the meshComponent EntityComponentIdPair matches")
        BaseClass.check_result(not(meshComponent.to_string() == ""), "EntityComponentIdPair to_string works")

        hasComponent = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', newEntityId, meshComponentTypeId)
        BaseClass.check_result(hasComponent, "Entity has a Mesh component")

        isActive = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', meshComponent)
        BaseClass.check_result(isActive, "Mesh component is active")

        editor.EditorComponentAPIBus(bus.Broadcast, 'DisableComponents', [meshComponent])

        isNotActive = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', meshComponent)

        BaseClass.check_result(not(isNotActive), "Mesh component is not active")
        BaseClass.check_result(editor.EditorComponentAPIBus(bus.Broadcast, 'IsValid', meshComponent), "Mesh component is valid")

        commentComponentsOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', newEntityId, [commentComponentTypeId, commentComponentTypeId])

        BaseClass.check_result(commentComponentsOutcome.IsSuccess(), "Comment components added to entity")

        commentComponents = commentComponentsOutcome.GetValue()

        getCommentComponentsOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentsOfType', newEntityId, commentComponentTypeId)
        BaseClass.check_result(getCommentComponentsOutcome.IsSuccess(), "GetComponentsOfType")
        getCommentComponents = getCommentComponentsOutcome.GetValue()

        getComponentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', newEntityId, meshComponentTypeId)

        BaseClass.check_result(getComponentOutcome.IsSuccess(), "GetComponentOfType")
        BaseClass.check_result(CompareComponentEntityIdPairs(getComponentOutcome.GetValue(), meshComponent), "CompareComponentEntityIdPairs meshComponent")

        commentsCount = editor.EditorComponentAPIBus(bus.Broadcast, 'CountComponentsOfType', newEntityId, commentComponentTypeId)

        BaseClass.check_result(commentsCount == 2, "Entity has two Comment components")

        editor.EditorComponentAPIBus(bus.Broadcast, 'DisableComponents', commentComponents)
        isCActive = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', commentComponents[0])
        isC2Active = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', commentComponents[1])

        BaseClass.check_result(not(isCActive) and not(isC2Active), "Disabled both Comment components")

        editor.EditorComponentAPIBus(bus.Broadcast, 'EnableComponents', commentComponents)
        isCActive = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', commentComponents[0])
        isC2Active = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', commentComponents[1])

        BaseClass.check_result(isCActive and isC2Active, "Enabled both Comment components")

        editor.EditorComponentAPIBus(bus.Broadcast, 'RemoveComponents', [meshComponent])

        hasMesh = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', newEntityId, meshComponentTypeId)

        componentSingleOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentOfType', newEntityId, commentComponentTypeId)

        BaseClass.check_result(componentSingleOutcome.IsSuccess(), "Single comment component added to entity")

        commentsCount = editor.EditorComponentAPIBus(bus.Broadcast, 'CountComponentsOfType', newEntityId, commentComponentTypeId)

        BaseClass.check_result(commentsCount == 3, "Entity has three Comment components")
        BaseClass.check_result(not(hasMesh), "Mesh Component removed")
        BaseClass.check_result(editor.EditorComponentAPIBus(bus.Broadcast, 'IsValid', meshComponent) is False, "Mesh component is NOT valid")

        # Test that it is possible to access Components with no Editor Component (for example, the PhysX Mesh Collider)

        meshColliderComponentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', newEntityId, [meshColliderComponentTypeId])
        BaseClass.check_result(meshColliderComponentOutcome.IsSuccess(), "PhysX Mesh Collider component added to entity")

        meshColliderComponent = meshColliderComponentOutcome.GetValue()
        getMeshColliderComponentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', newEntityId, meshColliderComponentTypeId)

        BaseClass.check_result(getMeshColliderComponentOutcome.IsSuccess(), f"getMeshColliderComponentOutcome.IsSuccess()")
        BaseClass.check_result(CompareComponentEntityIdPairs(meshColliderComponent[0], getMeshColliderComponentOutcome.GetValue()), "PhysX Mesh Collider component retrieved from entity")

        editor.EditorComponentAPIBus(bus.Broadcast, 'RemoveComponents', [meshColliderComponent[0]])
        hasMeshCollider = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', newEntityId, meshColliderComponentTypeId)
        BaseClass.check_result(not(hasMeshCollider), "PhysX Mesh Collider Component removed")

        # Test that it is possible to access Components with no Editor Component(for example, the PhysX Mesh Collider) via GetComponentOfType

        meshColliderComponentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', newEntityId, [meshColliderComponentTypeId])
        BaseClass.check_result(meshColliderComponentOutcome.IsSuccess(), "PhysX Mesh Collider component added to entity")

        meshColliderComponent = meshColliderComponentOutcome.GetValue()[0]
        getMeshColliderComponentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', newEntityId, meshColliderComponentTypeId)

        BaseClass.check_result(getMeshColliderComponentOutcome.IsSuccess(), "getMeshColliderComponentOutcome.IsSuccess()")
        BaseClass.check_result(CompareComponentEntityIdPairs(meshColliderComponent, getMeshColliderComponentOutcome.GetValue()), "PhysX Mesh Collider component retrieved from entity")

        meshColliderRemoved = False
        meshColliderRemoved = editor.EditorComponentAPIBus(bus.Broadcast, 'RemoveComponents', [meshColliderComponent])
        BaseClass.check_result(meshColliderRemoved, "PhysX Mesh Collider component removed from entity")

if __name__ == "__main__":
    tester = Editor_ComponentCommands_Works()
    tester.test_case(tester.test)
