"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

C16877221: Component List Filtering
https://testrail.agscollab.com/index.php?/cases/view/16877221
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import Tests.ly_shared.pyside_utils as pyside_utils
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as entity
import azlmbr.math as math
from PySide2 import QtWidgets, QtTest, QtCore
from PySide2.QtCore import Qt
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class ComponentListFilteringTest(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="component_list_filtering", args=["level"])

    @pyside_utils.wrap_async
    async def run_test(self):
        """
        Summary:
        Select an entity. Click "Add Component" in the Entity Inspector.
        Type the name of a valid component in the component search field, clear the search field and finally 
        type an invalid component name.
        The test verifies the component list is filtered correctly in all these cases.

        Expected Behavior:
        1) Components are filtered as you type narrowing down to the desired component.
        2) Components are filtered with each character deleted, eventually restoring the entire component list when the
        field is empty.
        3) Components are filtered until there are no entries present.

        Test Steps:
        1) Open level
        2) Create entity
        3) Select the newly created entity
        4) Click "Add Component" in the Entity Inspector.
        5) Type component name in search field.
        6) Verify list of components are filtered per the typed component name.
        7) Delete text in search field.
        8) Verify the list is restored.
        9) Type an invalid component name in search field.
        10) Verify the component list is empty as expected.

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        async def search_component(component_name, expected_row_count=None):
            # Click the "Add Component" button in the Entity Inspector and
            # then enter the specified `component_name` into its search field
            pyside_utils.click_button_async(add_comp_btn)
            popup = await pyside_utils.wait_for_popup_widget()
            search_frame = popup.findChild(QtWidgets.QFrame, "SearchFrame")
            search_text = search_frame.findChild(QtWidgets.QLineEdit, "SearchText")
            search_text.setText(component_name)

            # To ensure we get an updated tree after the component name is set, we
            # need to wait until the rowCount matches what we expect, since the
            # ComponentPaletteWidget::UpdateSearch is queued.
            def row_count_matches():
                tree = popup.findChild(QtWidgets.QTreeView, "Tree")
                row_count = tree.model().rowCount()
                if expected_row_count is not None:
                    return row_count == expected_row_count

                # Else, if expected_row_count was left as None, then just wait until
                # the tree model has been populated with anything
                return row_count > 0

            await pyside_utils.wait_for_condition(row_count_matches)

            # Print out the result based on what is populated in the component palette tree
            tree = popup.findChild(QtWidgets.QTreeView, "Tree")
            row_count = tree.model().rowCount()
            component_index = pyside_utils.find_child_by_pattern(tree, text=component_name, type=QtCore.QModelIndex)
            if component_index and component_index.isValid():
                print(f"{component_name} : Valid filtered component list")
            elif row_count == 0:
                print(f"{component_name} : Empty component list")
            else:
                print("Full component list")

        # 1) Open level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )

        # Make sure the Entity Inspector is open
        general.open_pane('Entity Inspector')

        # 2) Create entity
        entity_id = editor.ToolsApplicationRequestBus(
            bus.Broadcast, "CreateNewEntity", entity.EntityId()
        )

        # 3) Select the newly created entity
        editor.ToolsApplicationRequestBus(bus.Broadcast, 'SetSelectedEntities', [entity_id])
        # Give the Entity Inspector time to fully create its contents
        general.idle_wait(0.0)

        # 4) Click Add Component
        editor_window = pyside_utils.get_editor_main_window()
        entity_inspector = editor_window.findChild(QtWidgets.QDockWidget, "Entity Inspector")
        add_comp_btn = entity_inspector.findChild(QtWidgets.QPushButton, "m_addComponentButton")

        # 5,6) Search for a valid component name, verify list is properly filtered
        component_name_to_search = "Box Shape"
        await search_component(component_name_to_search, 1)

        # 7,8) Delete the search box, verify full list is loaded
        component_name_to_search = ""
        await search_component(component_name_to_search)

        # 9,10) Search for an invalid component name, verify the list is empty
        component_name_to_search = "Invalid Component Name !@#$"
        await search_component(component_name_to_search, 0)

test = ComponentListFilteringTest()
test.run()
