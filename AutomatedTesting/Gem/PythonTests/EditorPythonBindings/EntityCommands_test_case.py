"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Tests the Python API for Entity CRUD while the Editor is running

import azlmbr.bus as bus
import azlmbr.editor as editor
from azlmbr.entity import EntityId

createdEntityIds = []


def onEditorEntityCreated(parameters):
    global createdEntityIds

    entityId = parameters[0]
    createdEntityIds.append(entityId)


def onEditorEntityDeleted(parameters):
    global createdEntityIds

    deletedEntityId = parameters[0]
    for entityId in createdEntityIds:
        if (entityId.invoke("Equal", deletedEntityId)):
            createdEntityIds.remove(entityId)
            break


# Open a level (any level should work)
editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'OpenLevelNoPrompt', 'Base')

# Listen for notifications when entities are created/deleted
handler = bus.NotificationHandler('EditorEntityContextNotificationBus')
handler.connect()
handler.add_callback('OnEditorEntityCreated', onEditorEntityCreated)
handler.add_callback('OnEditorEntityDeleted', onEditorEntityDeleted)

# Create a new Entity at the root level
rootEntityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', EntityId())
if (rootEntityId):
    print("New entity with no parent created")
if (rootEntityId.invoke("Equal", createdEntityIds[0])):
    print("New root entity matches entity received from notification")

# Create a new Entity parented to the first Entity we created
firstChildEntityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', rootEntityId)
if (firstChildEntityId):
    print("New entity with valid parent created")
if (firstChildEntityId.invoke("Equal", createdEntityIds[1])):
    print("First child entity matches entity received from notification")

# Create another Entity parented to the first Entity we created
secondChildEntityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', rootEntityId)
if (secondChildEntityId):
    print("Another new entity with valid parent created")
if (secondChildEntityId.invoke("Equal", createdEntityIds[2])):
    print("Second child entity matches entity received from notification")

# Create two more entities and then delete them using the API that takes a list instead of a single entity
thirdChildEntityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', secondChildEntityId)
fourthChildEntityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', secondChildEntityId)
editor.ToolsApplicationRequestBus(bus.Broadcast, 'DeleteEntities', [thirdChildEntityId, fourthChildEntityId])

# Delete the second child Entity we created
editor.ToolsApplicationRequestBus(bus.Broadcast, 'DeleteEntityById', secondChildEntityId)

# Delete the root Entity we created and all its children (so this should also delete the firstChildEntityId)
editor.ToolsApplicationRequestBus(bus.Broadcast, 'DeleteEntityAndAllDescendants', rootEntityId)

# There should be no more entities left that we created
if (len(createdEntityIds) == 0):
    print("Deleted all entities we created")

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

if not(oldName == newName):
    print("GetName and SetName work")

# Create new Entity
parentId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', EntityId())
 
# Create new Entity with parentId as parent
childId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', parentId)

getId = editor.EditorEntityInfoRequestBus(bus.Event, 'GetParent', childId);

if(getId.Equal(parentId)):
    print("GetParent works")

# Find the entity in the scene

import azlmbr.bus as bus
import azlmbr.entity

searchFilter = azlmbr.entity.SearchFilter()
searchFilter.names = ['TestName']

# Search by name
searchEntityIdList = azlmbr.entity.SearchBus(bus.Broadcast, 'SearchEntities', searchFilter)

for entityId in searchEntityIdList:
    entityName = editor.EditorEntityInfoRequestBus(bus.Event, 'GetName', entityId)
    if(entityName == 'TestName'):
        print("TestName entity found")

# Search by name path (DAG)
editor.EditorEntityAPIBus(bus.Event, 'SetName', parentId, "TestParent")
editor.EditorEntityAPIBus(bus.Event, 'SetName', childId, "TestChild")

searchFilter = azlmbr.entity.SearchFilter()
searchFilter.names = ['TestParent|TestChild']

searchEntityIdList = azlmbr.entity.SearchBus(bus.Broadcast, 'SearchEntities', searchFilter)

for entityId in searchEntityIdList:
    entityName = editor.EditorEntityInfoRequestBus(bus.Event, 'GetName', entityId)
    if(entityName == 'TestChild'):
        print("TestChild entity found")

# Search using wildcard
searchFilter = azlmbr.entity.SearchFilter()
searchFilter.names = ['Test*']

searchEntityIdList = azlmbr.entity.SearchBus(bus.Broadcast, 'SearchEntities', searchFilter)

if(len(searchEntityIdList) == 3):
    print("Test* 3 entities found")

# Close Editor
editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'ExitNoPrompt')
