"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.


C2202976: Entity Selection - Entity Outliner
https://testrail.agscollab.com/index.php?/cases/view/2202976
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import azlmbr.entity as entity
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.slice as slice
import azlmbr.asset as asset
import azlmbr.math as math
import Tests.ly_shared.hydra_editor_utils as hydra
import Tests.ly_shared.pyside_utils as pyside_utils
from PySide2 import QtCore, QtWidgets, QtGui
from re import compile as regex
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class EntitySelectionEntityOutlinerTest(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="entity_selection_entity_outliner", args=["level"])

    def run_test(self):
        """
        Summary:
        Entity Selection - Entity Outliner - We need to verify if the Entity outliner works as expected
        for the entity selections by verifying the selected items.

        Expected Behavior:
        Single clicking entities will swap selection between entities.
        CTRL+Clicking entities will add to/remove from the selection group.

        Test Steps:
        1) Open level
        2) Create 3 new entities in the outliner
        3) Get Model Index for all the entities
        4) Verify single click
        5) Verify CTRL+click

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def verify_selected_index(expected_entity_list):
            # NOTE: selectedIndexes() seems to return 2 extra elements for each item seleted whose data() is None
            # So removing the items whose data is None and considering only the selected items having data.
            selected_indexes = [i.data() for i in tree.selectedIndexes() if i.data()]
            return sorted(selected_indexes) == sorted(expected_entity_list)

        # 1) Open level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )
        general.idle_wait(3.0)

        # Get the editor window object
        editor_window = pyside_utils.get_editor_main_window()

        # Get the outliner widget object based on the window title
        outliner_widget = pyside_utils.find_child_by_hierarchy(
            editor_window, ..., dict(windowTitle=regex("Entity Outliner.*")), ..., "m_objectList"
        ).parent()

        # Get the object tree in the entity outliner
        tree = pyside_utils.find_child_by_hierarchy(outliner_widget, ..., "m_objectTree")

        # 2) Create 3 new entities in the outliner
        entity_postition = math.Vector3(125.0, 136.0, 32.0)
        entity_list = ["Entity2", "Entity3", "Entity4"]
        [hydra.Entity(name).create_entity(entity_postition, []) for name in entity_list]

        # 3) Get Model Index for all the entities
        model_index_2, model_index_3, model_index_4 = [
            pyside_utils.find_child_by_pattern(tree, name) for name in entity_list
        ]

        # 4) Verify single click
        # click on Entity2 and verify if it is selected
        pyside_utils.item_view_index_mouse_click(tree, model_index_2)
        print(f"Single entity selected on first click: {verify_selected_index(['Entity2'])}")

        # click on Entity3 and verify if only that is selected
        pyside_utils.item_view_index_mouse_click(tree, model_index_3)
        print(f"Single entity selected on second click: {verify_selected_index(['Entity3'])}")

        # 5) Verify CTRL+click
        # CTRL+click on Entity2 and verify if Entity2 and Entity3 are selected as Entity is already selected
        pyside_utils.item_view_index_mouse_click(tree, model_index_2, modifier=QtCore.Qt.ControlModifier)
        print(
            f"CTRL+click worked for adding selected elements (2 elements): {verify_selected_index(['Entity2', 'Entity3'])}"
        )

        # CTRL+click on Entity4 and verify if Entity 2, 3, 4 are selected
        pyside_utils.item_view_index_mouse_click(tree, model_index_4, modifier=QtCore.Qt.ControlModifier)
        print(
            f"CTRL+click worked for adding selected elements (3 elements): {verify_selected_index(['Entity2', 'Entity3', 'Entity4'])}"
        )

        # CTRL+click on already selected element (Entity2) to verify if it is removed from selected items
        pyside_utils.item_view_index_mouse_click(tree, model_index_2, modifier=QtCore.Qt.ControlModifier)
        print(
            f"CTRL+click worked for removing already selected elements: {verify_selected_index(['Entity3', 'Entity4'])}"
        )
        
test = EntitySelectionEntityOutlinerTest()
test.run()
