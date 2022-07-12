"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os, sys
sys.path.append(os.path.dirname(__file__))
from Editor_TestClass import BaseClass

class Editor_EntitySelectionCommands_Works(BaseClass):
    # Description: 
    # Tests the Entity Selection Python API while the Editor is running
    
    @staticmethod
    def test():
        import azlmbr.bus as bus
        import azlmbr.editor as editor
        from azlmbr.entity import EntityId
        check_result = BaseClass.check_result

        def createTestEntities(count):
            testEntityIds = []
    
            for i in range(count):
                entityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', EntityId())
                testEntityIds.append(entityId)
                print("Entity " + str(i)  + " created.")
                editor.EditorEntityAPIBus(bus.Event, 'SetName', entityId, "TestEntity")

            return testEntityIds

        # Create new test entities at the root level
        numTestEntities = 10
        testEntityIds = createTestEntities(numTestEntities)

        # Select all test entities
        editor.ToolsApplicationRequestBus(bus.Broadcast, 'SetSelectedEntities', testEntityIds)

        # Get all test entities selected and check if any entity selected
        selectedTestEntityIds = editor.ToolsApplicationRequestBus(bus.Broadcast, 'GetSelectedEntities')
        areAnyEntitiesSelected = editor.ToolsApplicationRequestBus(bus.Broadcast, 'AreAnyEntitiesSelected')

        check_result(len(testEntityIds) == len(selectedTestEntityIds) and areAnyEntitiesSelected, "All Test Entities Selected")

        # Mark first test entity deselected
        editor.ToolsApplicationRequestBus(bus.Broadcast, 'MarkEntityDeselected', testEntityIds[0])
        selectedTestEntityIds = editor.ToolsApplicationRequestBus(bus.Broadcast, 'GetSelectedEntities')
        isSelected = editor.ToolsApplicationRequestBus(bus.Broadcast, 'IsSelected', testEntityIds[0])

        check_result(len(testEntityIds) - 1 == len(selectedTestEntityIds) and not isSelected, "One Test Entity Marked as DeSelected")

        # Mark first test entity selected
        editor.ToolsApplicationRequestBus(bus.Broadcast, 'MarkEntitySelected', testEntityIds[0])
        selectedTestEntityIds = editor.ToolsApplicationRequestBus(bus.Broadcast, 'GetSelectedEntities')
        isSelected = editor.ToolsApplicationRequestBus(bus.Broadcast, 'IsSelected', testEntityIds[0])

        check_result(len(testEntityIds) == len(selectedTestEntityIds) and isSelected, "One New Test Entity Marked as Selected")

        # Mark first half of test entities as deselected
        halfNumTestEntities = len(testEntityIds)//2

        editor.ToolsApplicationRequestBus(bus.Broadcast, 'MarkEntitiesDeselected', testEntityIds[:halfNumTestEntities])
        selectedTestEntityIds = editor.ToolsApplicationRequestBus(bus.Broadcast, 'GetSelectedEntities')

        check_result(len(testEntityIds) - halfNumTestEntities == len(selectedTestEntityIds), "Half of Test Entity Marked as DeSelected")

        # Mark first half test entity as selected
        editor.ToolsApplicationRequestBus(bus.Broadcast, 'MarkEntitiesSelected', testEntityIds[:halfNumTestEntities])
        selectedTestEntityIds = editor.ToolsApplicationRequestBus(bus.Broadcast, 'GetSelectedEntities')

        check_result(len(testEntityIds) == len(selectedTestEntityIds), "All Test Entities Marked as Selected")

        # Clear all test entities selected
        editor.ToolsApplicationRequestBus(bus.Broadcast, 'SetSelectedEntities', [])
        selectedTestEntityIds = editor.ToolsApplicationRequestBus(bus.Broadcast, 'GetSelectedEntities')
        areAnyEntitiesSelected = editor.ToolsApplicationRequestBus(bus.Broadcast, 'AreAnyEntitiesSelected')

        check_result(len(selectedTestEntityIds) == 0 and not areAnyEntitiesSelected, "Clear All Test Entities Selected")

if __name__ == "__main__":
    tester = Editor_EntitySelectionCommands_Works()
    tester.test_case(tester.test)
