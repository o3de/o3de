"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
def check_result(msg, result):
    from editor_python_test_tools.utils import Report
    if not result:
        Report.result(msg, False)
        raise Exception(msg + " : FAILED")

def Editor_EntityCommands_Works():
    # Description: 
    # Tests the Python API for Entity CRUD while the Editor is running

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper
    import azlmbr.bus as bus
    import azlmbr.editor as editor
    from azlmbr.entity import EntityId
    import azlmbr.legacy.general    
    import azlmbr.entity

    # Required for automated tests
    TestHelper.init_idle()

    createdEntityIds = []

    def onEditorEntityCreated(parameters):
        entityId = parameters[0]
        createdEntityIds.append(entityId)


    def onEditorEntityDeleted(parameters):
        deletedEntityId = parameters[0]
        for entityId in createdEntityIds:
            if (entityId.invoke("Equal", deletedEntityId)):
                createdEntityIds.remove(entityId)
                break

    # Listen for notifications when entities are created/deleted
    handler = bus.NotificationHandler('EditorEntityContextNotificationBus')
    handler.connect()
    handler.add_callback('OnEditorEntityCreated', onEditorEntityCreated)
    handler.add_callback('OnEditorEntityDeleted', onEditorEntityDeleted)

    # Open the test level
    TestHelper.open_level(directory="", level="Base")
    azlmbr.legacy.general.idle_wait_frames(1)

    # Create a new Entity at the root level
    rootEntityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', EntityId())
    check_result("New entity with no parent created", rootEntityId)
    check_result("New root entity matches entity received from notification", rootEntityId.invoke("Equal", createdEntityIds[0]))

    # Create a new Entity parented to the first Entity we created
    firstChildEntityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', rootEntityId)
    check_result("New entity with valid parent created", firstChildEntityId)
    check_result("First child entity matches entity received from notification", firstChildEntityId.invoke("Equal", createdEntityIds[1]))

    # Create another Entity parented to the first Entity we created
    secondChildEntityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', rootEntityId)
    check_result("Another new entity with valid parent created", secondChildEntityId)
    check_result("Second child entity matches entity received from notification", secondChildEntityId.invoke("Equal", createdEntityIds[2]))

    # Create two more entities and then delete them using the API that takes a list instead of a single entity
    thirdChildEntityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', secondChildEntityId)
    fourthChildEntityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', secondChildEntityId)
    editor.ToolsApplicationRequestBus(bus.Broadcast, 'DeleteEntities', [thirdChildEntityId, fourthChildEntityId])

    # Delete the second child Entity we created
    editor.ToolsApplicationRequestBus(bus.Broadcast, 'DeleteEntityById', secondChildEntityId)

    # Delete the root Entity we created and all its children (so this should also delete the firstChildEntityId)
    editor.ToolsApplicationRequestBus(bus.Broadcast, 'DeleteEntityAndAllDescendants', rootEntityId)

    # There should be no more entities left that we created
    check_result("Deleted all entities we created", len(createdEntityIds) == 0)

    # Stop listening for entity creation/deletion notifications
    handler.disconnect()

    # Create new Entity
    entityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', EntityId())

    # Get current name
    oldName = editor.EditorEntityInfoRequestBus(bus.Event, 'GetName', entityId);

    # Set a new name
    editor.EditorEntityAPIBus(bus.Event, 'SetName', entityId, "TestName")

    # Get new name
    newName = editor.EditorEntityInfoRequestBus(bus.Event, 'GetName', entityId);
    check_result("GetName and SetName work", not(oldName == newName))

    # Create new Entity
    parentId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', EntityId())
 
    # Create new Entity with parentId as parent
    childId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', parentId)

    getId = editor.EditorEntityInfoRequestBus(bus.Event, 'GetParent', childId);
    check_result("GetParent works", getId.Equal(parentId))

    # Find the entity in the scene

    searchFilter = azlmbr.entity.SearchFilter()
    searchFilter.names = ['TestName']

    # Search by name
    searchEntityIdList = azlmbr.entity.SearchBus(bus.Broadcast, 'SearchEntities', searchFilter)

    for entityId in searchEntityIdList:
        entityName = editor.EditorEntityInfoRequestBus(bus.Event, 'GetName', entityId)
        check_result("TestName entity found", entityName == 'TestName')

    # Search by name path (DAG)
    editor.EditorEntityAPIBus(bus.Event, 'SetName', parentId, "TestParent")
    editor.EditorEntityAPIBus(bus.Event, 'SetName', childId, "TestChild")

    searchFilter = azlmbr.entity.SearchFilter()
    searchFilter.names = ['TestParent|TestChild']

    searchEntityIdList = azlmbr.entity.SearchBus(bus.Broadcast, 'SearchEntities', searchFilter)

    for entityId in searchEntityIdList:
        entityName = editor.EditorEntityInfoRequestBus(bus.Event, 'GetName', entityId)
        check_result("TestChild entity found", entityName == 'TestChild')

    # Search using wildcard
    searchFilter = azlmbr.entity.SearchFilter()
    searchFilter.names = ['Test*']

    searchEntityIdList = azlmbr.entity.SearchBus(bus.Broadcast, 'SearchEntities', searchFilter)
    check_result("Test* 3 entities found", len(searchEntityIdList) == 3)
    
    # all tests worked
    Report.result("Editor_EntityCommands_Works ran", True)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Editor_EntityCommands_Works)








