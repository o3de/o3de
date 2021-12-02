"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Tests the Entity Selection Python API while the Editor is running

import azlmbr.bus as bus
import azlmbr.editor as editor
from azlmbr.entity import EntityId

def createTestEntities(count):
    testEntityIds = []
    
    for i in range(count):
        entityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', EntityId())
        testEntityIds.append(entityId)
        print("Entity " + str(i)  + " created.")
        editor.EditorEntityAPIBus(bus.Event, 'SetName', entityId, "TestEntity")

    return testEntityIds

# Open a level (any level should work)
editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'OpenLevelNoPrompt', 'WaterSample')


# Create new test entities at the root level
numTestEntities = 10
testEntityIds = createTestEntities(numTestEntities)

# Select all test entities
editor.ToolsApplicationRequestBus(bus.Broadcast, 'SetSelectedEntities', testEntityIds)

# Get all test entities selected and check if any entity selected
selectedTestEntityIds = editor.ToolsApplicationRequestBus(bus.Broadcast, 'GetSelectedEntities')
areAnyEntitiesSelected = editor.ToolsApplicationRequestBus(bus.Broadcast, 'AreAnyEntitiesSelected')

if(len(testEntityIds) == len(selectedTestEntityIds) and areAnyEntitiesSelected): 
    print("SUCCESS: All Test Entities Selected")

# Mark first test entity deselected
editor.ToolsApplicationRequestBus(bus.Broadcast, 'MarkEntityDeselected', testEntityIds[0])
selectedTestEntityIds = editor.ToolsApplicationRequestBus(bus.Broadcast, 'GetSelectedEntities')
isSelected = editor.ToolsApplicationRequestBus(bus.Broadcast, 'IsSelected', testEntityIds[0])

if(len(testEntityIds) - 1 == len(selectedTestEntityIds) and not isSelected): 
    print("SUCCESS: One Test Entity Marked as DeSelected")

# Mark first test entity selected
editor.ToolsApplicationRequestBus(bus.Broadcast, 'MarkEntitySelected', testEntityIds[0])
selectedTestEntityIds = editor.ToolsApplicationRequestBus(bus.Broadcast, 'GetSelectedEntities')
isSelected = editor.ToolsApplicationRequestBus(bus.Broadcast, 'IsSelected', testEntityIds[0])

if(len(testEntityIds) == len(selectedTestEntityIds) and isSelected): 
    print("SUCCESS: One New Test Entity Marked as Selected")

# Mark first half of test entities as deselected
halfNumTestEntities = len(testEntityIds)//2

editor.ToolsApplicationRequestBus(bus.Broadcast, 'MarkEntitiesDeselected', testEntityIds[:halfNumTestEntities])
selectedTestEntityIds = editor.ToolsApplicationRequestBus(bus.Broadcast, 'GetSelectedEntities')

if(len(testEntityIds) - halfNumTestEntities == len(selectedTestEntityIds)): 
    print("SUCCESS: Half of Test Entity Marked as DeSelected")

# Mark first half test entity as selected
editor.ToolsApplicationRequestBus(bus.Broadcast, 'MarkEntitiesSelected', testEntityIds[:halfNumTestEntities])
selectedTestEntityIds = editor.ToolsApplicationRequestBus(bus.Broadcast, 'GetSelectedEntities')

if(len(testEntityIds) == len(selectedTestEntityIds)): 
    print("SUCCESS: All Test Entities Marked as Selected")

# Clear all test entities selected
editor.ToolsApplicationRequestBus(bus.Broadcast, 'SetSelectedEntities', [])
selectedTestEntityIds = editor.ToolsApplicationRequestBus(bus.Broadcast, 'GetSelectedEntities')
areAnyEntitiesSelected = editor.ToolsApplicationRequestBus(bus.Broadcast, 'AreAnyEntitiesSelected')

if(len(selectedTestEntityIds) == 0 and not areAnyEntitiesSelected): 
    print("SUCCESS: Clear All Test Entities Selected")


# Close Editor
editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'ExitNoPrompt')
