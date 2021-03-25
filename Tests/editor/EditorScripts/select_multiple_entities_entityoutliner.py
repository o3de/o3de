"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

"""
C1508364 : Select over 1,000+ entities that are open in the Entity Outliner at one time
https://testrail.agscollab.com/index.php?/cases/view/1508364
"""

import azlmbr.legacy.general as general
import azlmbr.entity as entity
import azlmbr.bus as bus
import azlmbr.editor as editor
from azlmbr.entity import EntityId
import Tests.ly_shared.pyside_utils as pyside_utils
from PySide2 import QtCore, QtTest, QtWidgets
from PySide2.QtCore import Qt
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper
from re import compile as regex


class TestSelectMultipleEntitiesEntityOutliner(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="select_multiple_entities_entityoutliner", args=["level"])

    def run_test(self):
        """
        Summary:
        Select over 1,000+ entities that are open in the Entity Outliner at one time

        Expected Behavior:
        The engine remains stable while selecting a large number of entities.
        Selected entities are highlighted in the Entity Outliner.
        The Entity Inspector shows an accurate count of selected entities.
        The engine remains stable while selecting entities in the Outliner.
        Selected entities are highlighted in the outliner.
        The Entity Inspector shows an accurate count of selected entities.

        Test Steps:
         1) Open a new level
         2) Delete the DefaultLevelSetup initially to make the selection of entities easier
         3) Create 1000 entities using 'Duplicate' action
         4) Verify CTRL+click to select individual entities
         5) Select all entities and make sure selected entities are highlighted in the Entity Outliner
         6) Verify the count of selected entities in Entity Inspector

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def get_entity_count_name(entity_name):
            searchFilter = entity.SearchFilter()
            searchFilter.names = [entity_name]
            entities = entity.SearchBus(bus.Broadcast, "SearchEntities", searchFilter)
            return len(entities)

        def verify_selected_index(expected_entity_list):
            # NOTE: selectedIndexes() seems to return 2 extra elements for each item seleted whose data() is None
            # So removing the items whose data is None and considering only the selected items having data.
            selected_indexes = [i for i in tree.selectedIndexes() if i.data()]
            return sorted(selected_indexes) == sorted(expected_entity_list)

        # 1) Open a new level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )

        # Get the editor window object
        editor_window = pyside_utils.get_editor_main_window()

        # Get the outliner widget object based on the window title
        outliner_widget = editor_window.findChildren(QtWidgets.QDockWidget, "Entity Outliner (PREVIEW)")[0].widget()

        # Get the entity inspector object based on the window title
        entity_inspector_name_editor = pyside_utils.find_child_by_hierarchy(
            editor_window, ..., dict(windowTitle=regex("Entity Inspector.*")), ..., "m_entityNameEditor"
        )

        # Get the object tree in the entity outliner
        tree = pyside_utils.find_child_by_hierarchy(outliner_widget, ..., "m_objectTree")

        # 2) Delete the DefaultLevelSetup initially to make the selection of entities easier
        default_entity_id = general.find_editor_entity("DefaultLevelSetup")
        editor.ToolsApplicationRequestBus(bus.Broadcast, "DeleteEntities", [default_entity_id])

        # 3) Create 1000 entities using 'Duplicate' action
        entity_id = editor.ToolsApplicationRequestBus(bus.Broadcast, "CreateNewEntity", EntityId())
        entity_name = editor.EditorEntityInfoRequestBus(bus.Event, "GetName", entity_id)
        editor.ToolsApplicationRequestBus(bus.Broadcast, "SetSelectedEntities", [entity_id])
        for _ in range(999):
            QtTest.QTest.keyPress(outliner_widget, Qt.Key_D, Qt.ControlModifier)

        if get_entity_count_name(entity_name) == 1000:
            print("1000 entities created")
        editor.ToolsApplicationRequestBus(bus.Broadcast, "SetSelectedEntities", [])

        # 4) Verify CTRL+click to select individual entities
        # CTRL+click on First Entity and verify if First Entity is selected
        self.wait_for_condition(lambda: pyside_utils.get_item_view_index(tree, 1) is not None, 2.0)
        model_index_1 = pyside_utils.get_item_view_index(tree, 0)
        pyside_utils.item_view_index_mouse_click(tree, model_index_1, modifier=QtCore.Qt.ControlModifier)
        self.wait_for_condition(lambda: entity_inspector_name_editor.text() == "Entity1", 1.0)
        print(
            f"The Entity Inspector shows an accurate count of selected entities (1 element): {entity_inspector_name_editor.text() == 'Entity1'}"
        )
        print(f"CTRL+click worked for adding selected elements (1 element): {verify_selected_index([model_index_1])}")

        # CTRL+click on Second Entity and verify if First Entity and Second Entity are selected
        self.wait_for_condition(lambda: pyside_utils.get_item_view_index(tree, 2) is not None, 2.0)
        model_index_2 = pyside_utils.get_item_view_index(tree, 1)
        pyside_utils.item_view_index_mouse_click(tree, model_index_2, modifier=QtCore.Qt.ControlModifier)
        self.wait_for_condition(lambda: entity_inspector_name_editor.text() == "2 entities selected", 1.0)
        print(
            f"The Entity Inspector shows an accurate count of selected entities (2 elements): {entity_inspector_name_editor.text() == '2 entities selected'}"
        )
        print(
            f"CTRL+click worked for adding selected elements (2 elements): {verify_selected_index([model_index_1, model_index_2])}"
        )

        # CTRL+click on Third Entity and verify if First Entity, Second Entity and Third Entity are selected
        self.wait_for_condition(lambda: pyside_utils.get_item_view_index(tree, 3) is not None, 2.0)
        model_index_3 = pyside_utils.get_item_view_index(tree, 2)
        pyside_utils.item_view_index_mouse_click(tree, model_index_3, modifier=QtCore.Qt.ControlModifier)
        self.wait_for_condition(lambda: entity_inspector_name_editor.text() == "3 entities selected", 1.0)
        print(
            f"The Entity Inspector shows an accurate count of selected entities (3 elements): {entity_inspector_name_editor.text() == '3 entities selected'}"
        )
        print(
            f"CTRL+click worked for adding selected elements (3 elements): {verify_selected_index([model_index_1, model_index_2, model_index_3])}"
        )

        # 5) Select all entities and make sure selected entities are highlighted in the Entity Outliner
        action = pyside_utils.get_action_for_menu_path(editor_window, "Edit", "Select All")
        action.trigger()
        if len(editor.ToolsApplicationRequestBus(bus.Broadcast, "GetSelectedEntities")) == 1000:
            print("Selected entities are highlighted in the Entity Outliner")

        # 6) Verify the count of selected entities in Entity Inspector
        self.wait_for_condition(lambda: entity_inspector_name_editor.text() == "1000 entities selected", 1.0)
        print(
            f"The Entity Inspector shows an accurate count of selected entities (1000 elements): {entity_inspector_name_editor.text() == '1000 entities selected'}"
        )


test = TestSelectMultipleEntitiesEntityOutliner()
test.run()
