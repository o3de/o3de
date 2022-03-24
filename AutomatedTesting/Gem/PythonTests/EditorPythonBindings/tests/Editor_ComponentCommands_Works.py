"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
def AssertThat(result, msg):
    from editor_python_test_tools.utils import Report
    if result:
        Report.result(f"{msg}: SUCCESS", True)
    else:
        msg = msg + " :  FAILED"
        Report.result(msg, False)
        raise Exception(msg)

def Editor_ComponentCommands_Works():
    # Description: 
    # Tests a portion of the Component CRUD Python API while the Editor is running
    # It needs a new test level in prefab format to make it testable again.
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper
    import azlmbr.bus as bus
    import azlmbr.entity as entity
    import azlmbr.editor as editor
    import azlmbr.object
    import azlmbr.math
    from azlmbr.entity import EntityId

    # Required for automated tests
    TestHelper.init_idle()

    # Open the test level
    TestHelper.open_level(directory="", level="Base")
    azlmbr.legacy.general.idle_wait_frames(1)

    def CompareComponentEntityIdPairs(component1, component2):
        return component1.Equal(component2)

    # Open a level (any level should work)
    editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'OpenLevelNoPrompt', 'Base')

    # Get Component Types for Mesh and Comment
    typeIdsList = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', ["Mesh", "Comment", "PhysX Collider"], entity.EntityType().Game)

    AssertThat(len(typeIdsList) > 0, "Type Ids List returned correctly")

    meshComponentTypeId = typeIdsList[0]
    commentComponentTypeId = typeIdsList[1]
    meshColliderComponentTypeId = typeIdsList[2]

    # Get Component Ids from Component Types
    typeNamesList = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeNames', typeIdsList)

    AssertThat( (typeNamesList[0] == "Mesh") and (typeNamesList[1] == "Comment") and (typeNamesList[2] == "PhysX Collider"), 
           f"Type Names List returned correctly: {typeNamesList}")

    # Test Component API
    newEntityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', EntityId())

    AssertThat(newEntityId,  "New entity with no parent created")

    hadComponent = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', newEntityId, meshComponentTypeId)

    AssertThat(not(hadComponent), "Entity does not have a Mesh component")

    meshComponentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', newEntityId, [meshComponentTypeId])

    AssertThat(meshComponentOutcome.IsSuccess(), "Mesh component added to entity")

    meshComponents = meshComponentOutcome.GetValue()
    meshComponent = meshComponents[0]

    AssertThat(meshComponent.get_entity_id() == newEntityId, "EntityId on the meshComponent EntityComponentIdPair matches")
    AssertThat(not(meshComponent.to_string() == ""), "EntityComponentIdPair to_string works")

    hasComponent = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', newEntityId, meshComponentTypeId)

    AssertThat(hasComponent, "Entity has a Mesh component")

    isActive = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', meshComponent)

    AssertThat(isActive, "Mesh component is active")

    editor.EditorComponentAPIBus(bus.Broadcast, 'DisableComponents', [meshComponent])

    isNotActive = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', meshComponent)

    AssertThat(not(isNotActive), "Mesh component is not active")
    AssertThat(editor.EditorComponentAPIBus(bus.Broadcast, 'IsValid', meshComponent), "Mesh component is valid")

    CommentComponentsOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', newEntityId, [commentComponentTypeId, commentComponentTypeId])

    AssertThat(CommentComponentsOutcome.IsSuccess(), "Comment components added to entity")

    CommentComponents = CommentComponentsOutcome.GetValue()

    GetCommentComponentsOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentsOfType', newEntityId, commentComponentTypeId)

    if(GetCommentComponentsOutcome.IsSuccess()):
        GetCommentComponents = GetCommentComponentsOutcome.GetValue()

    AssertThat(CompareComponentEntityIdPairs(CommentComponents[0], GetCommentComponents[0]), "CommentComponents[0][0] compare")
    AssertThat(CompareComponentEntityIdPairs(CommentComponents[1], GetCommentComponents[1]), "CommentComponents[1][1] compare")
#   AssertThat(CompareComponentEntityIdPairs(CommentComponents[0], GetCommentComponents[1]), "CommentComponents[0][1] compare")
#   AssertThat(CompareComponentEntityIdPairs(CommentComponents[1], GetCommentComponents[0]), "CommentComponents[1][0] compare")

    GetComponentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', newEntityId, meshComponentTypeId)

    AssertThat(GetComponentOutcome.IsSuccess() and CompareComponentEntityIdPairs(GetComponentOutcome.GetValue(), meshComponent), "GetComponent works")

    commentsCount = editor.EditorComponentAPIBus(bus.Broadcast, 'CountComponentsOfType', newEntityId, commentComponentTypeId)

    AssertThat(commentsCount == 2, "Entity has two Comment components")

    editor.EditorComponentAPIBus(bus.Broadcast, 'DisableComponents', CommentComponents)
    isCActive = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', CommentComponents[0])
    isC2Active = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', CommentComponents[1])

    AssertThat(not(isCActive) and not(isC2Active), "Disabled both Comment components")

    editor.EditorComponentAPIBus(bus.Broadcast, 'EnableComponents', CommentComponents)
    isCActive = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', CommentComponents[0])
    isC2Active = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', CommentComponents[1])

    AssertThat(isCActive and isC2Active, "Enabled both Comment components")

    editor.EditorComponentAPIBus(bus.Broadcast, 'RemoveComponents', [meshComponent])

    hasMesh = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', newEntityId, meshComponentTypeId)

    componentSingleOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentOfType', newEntityId, commentComponentTypeId)

    AssertThat(componentSingleOutcome.IsSuccess(), "Single comment component added to entity")

    commentsCount = editor.EditorComponentAPIBus(bus.Broadcast, 'CountComponentsOfType', newEntityId, commentComponentTypeId)

    AssertThat(commentsCount == 3, "Entity has three Comment components")
    AssertThat(not(hasMesh), "Mesh Component removed")
    AssertThat(editor.EditorComponentAPIBus(bus.Broadcast, 'IsValid', meshComponent), "Mesh component is no longer valid")

    # Test that it is possible to access Components with no Editor Component (for example, the PhysX Collider)

    meshColliderComponentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', newEntityId, [meshColliderComponentTypeId])

    AssertThat(meshColliderComponentOutcome.IsSuccess(), "PhysX Collider component added to entity")

    meshColliderComponent = meshColliderComponentOutcome.GetValue()

    getMeshColliderComponentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', newEntityId, meshComponentTypeId)

    AssertThat(getMeshColliderComponentOutcome.IsSuccess() and CompareComponentEntityIdPairs(meshColliderComponent, getMeshColliderComponentOutcome.GetValue()),
        "PhysX Collider component retrieved from entity")

    editor.EditorComponentAPIBus(bus.Broadcast, 'RemoveComponents', [meshColliderComponent])

    hasMeshCollider = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', newEntityId, meshColliderComponentTypeId)

    AssertThat(not(hasMeshCollider), "PhysX Collider Component removed")

    # Test that it is possible to access Components with no Editor Component(for example, the PhysX Collider) via GetComponentOfType

    meshColliderComponentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', newEntityId, [meshColliderComponentTypeId])

    AssertThat(meshColliderComponentOutcome.IsSuccess(), "PhysX Collider component added to entity")

    meshColliderComponent = meshColliderComponentOutcome.GetValue()[0]

    getMeshColliderComponentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', newEntityId, meshColliderComponentTypeId)

    AssertThat(getMeshColliderComponentOutcome.IsSuccess() and CompareComponentEntityIdPairs(meshColliderComponent, getMeshColliderComponentOutcome.GetValue()),
           "PhysX Collider component retrieved from entity")

    meshColliderRemoved = False;
    meshColliderRemoved = editor.EditorComponentAPIBus(bus.Broadcast, 'RemoveComponents', [meshColliderComponent])

    AssertThat(meshColliderRemoved, "PhysX Collider component removed from entity")

    # all tests worked
    Report.result("Editor_ComponentCommands_Works succeeded.", True)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Editor_ComponentCommands_Works)



