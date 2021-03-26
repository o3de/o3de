"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

C1510643: Deleting Entities in the Entity Outliner
https://testrail.agscollab.com/index.php?/cases/view/1510643
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import Tests.ly_shared.pyside_utils as pyside_utils
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as EntityId
from PySide2 import QtWidgets
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class TestDeletingEntitiesFromOutliner(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="deleting_entities_from_outliner: ",  args=["level"])

    @pyside_utils.wrap_async
    async def run_test(self):
        """
        Summary:
        Create a nested group of Entities with parent and 2 children.
        Right click on each of the entities and select "Delete".
        Undo the deletion of those entites.

        Expected Behavior:
        Deleting a Child entity does not delete the parent.
        The child entity is restored as part of the hierarchy.
        Deleting the Parent entity deletes all child entities linked to it.
        The entity hierarchy is restored.

        Test Steps:
        1) Create a nested group of Entities and verify deleting the child does not destroy the parent
            1.1) Create group of entities
            1.2) Delete child
            1.3) Verify parent exists
        2) Undo the deletion of the child entity and verify the same is restored
            2.1) Undo the deletion
            2.2) verify child entity exists
        3) Delete the parent entity and verify all child entities deleted
            3.1) Delete parent
            3.2) Verify all child entities deleted
        4) Undo the deletion of the parent entity and verify hierarchy is restored
            4.1) Undo the deletion
            4.2) Verify all entities exist

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def get_entity_count_by_id(entity_id):
            entity_name = editor.EditorEntityInfoRequestBus(bus.Event, "GetName", entity_id)
            searchFilter = EntityId.SearchFilter()
            searchFilter.names = [entity_name]
            entities = EntityId.SearchBus(bus.Broadcast, "SearchEntities", searchFilter)
            return len(entities)

        async def delete_entity(entity_id, entity_name):
            """
            Delete an entity using PySide Right-Click and Delete.
            :param entity_id: used to find and delete the entity
            :param entity_name: not dynamically grabbed from entity or used in delete code but is used for printing out a comment statement for testrunner
            :return None
            """
            editor_window = pyside_utils.get_editor_main_window()
            main_window = editor_window.findChild(QtWidgets.QMainWindow)
            entity_outliner = main_window.findChild(QtWidgets.QDockWidget, "Entity Outliner (PREVIEW)")
            object_tree = entity_outliner.findChild(QtWidgets.QTreeView, "m_objectTree")
            index_to_delete = pyside_utils.find_child_by_pattern(object_tree, entity_name)
            object_tree.setCurrentIndex(index_to_delete)
            await pyside_utils.trigger_context_menu_entry(object_tree, "Delete", index=index_to_delete)

            if get_entity_count_by_id(entity_id) == 0:
                print(f"{entity_name} deleted succesfully")

        # Create a level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )

        # 1) Create a nested group of Entities and verify deleting the child does not destroy the parent
        # 1.1) Create group of entities
        parent_entity_id = editor.ToolsApplicationRequestBus(bus.Broadcast, "CreateNewEntity", EntityId.EntityId())
        editor.EditorEntityAPIBus(bus.Event, "SetName", parent_entity_id, "Parent entity")
        if get_entity_count_by_id(parent_entity_id) ==1:
            print("Parent Entity created")

        child_entity_1 = editor.ToolsApplicationRequestBus(bus.Broadcast, "CreateNewEntity", parent_entity_id)
        editor.EditorEntityAPIBus(bus.Event, "SetName", child_entity_1, "Child entity 1")
        child_entity_2 = editor.ToolsApplicationRequestBus(bus.Broadcast, "CreateNewEntity", parent_entity_id)
        editor.EditorEntityAPIBus(bus.Event, "SetName", child_entity_2, "Child entity 2")
        if get_entity_count_by_id(child_entity_1) == 1 and get_entity_count_by_id(child_entity_1) == 1:
            print("Child Entities created")

        # 1.2) Delete child
        await delete_entity(child_entity_1, "Child entity 1")

        # 1.3) Verify parent exists
        if get_entity_count_by_id(parent_entity_id) != 0:
            print("Deleting a Child entity does not delete the parent")

        # 2) Undo the deletion of the child entity and verify the same is restored
        # 2.1) Undo the deletion
        general.undo()

        # 2.2) verify child entity exists
        success = await pyside_utils.wait_for_condition(lambda: get_entity_count_by_id(child_entity_1) != 0)
        if success:
            print("Child entity restored successfully")

        # 3) Delete the parent entity and verify all child entities deleted
        # 3.1) Delete parent
        await delete_entity(parent_entity_id, "Parent entity")

        # 3.2) Verify all child entities deleted
        if get_entity_count_by_id(child_entity_1) == 0 and get_entity_count_by_id(child_entity_1) == 0:
            print("Deleting the Parent entity deletes all child entities linked to it")

        # 4) Undo the deletion of the parent entity and verify hierarchy is restored
        # 4.1) Undo the deletion
        general.undo()

        # 4.2) Verify all entities exist
        success = await pyside_utils.wait_for_condition(lambda:
            (get_entity_count_by_id(parent_entity_id) != 0
            and get_entity_count_by_id(child_entity_1) != 0
            and get_entity_count_by_id(child_entity_2) != 0))
        if success:
            print("Entity hierarchy is restored")


test = TestDeletingEntitiesFromOutliner()
test.run()
