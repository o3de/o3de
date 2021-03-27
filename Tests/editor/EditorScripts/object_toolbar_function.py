"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

C1564082 : Object Toolbar
https://testrail.agscollab.com/index.php?/cases/view/1564082
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as EntityId
import Tests.ly_shared.pyside_utils as pyside_utils
import Tests.ly_shared.hydra_editor_utils as editor_utils
from PySide2 import QtWidgets
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class TestObjectToolbar(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="object_toolbar_function: ", args=["level"])

    def run_test(self):
        """
        Summary:
        Interact with Object toolbar options and verify if all the options are functionnal.

        Expected Behavior:
        All options available in the Object Toolbar function as intended.

        Test Steps:
         1) Create/Open a level
         2) Open the Object Toolbar
         3) Verify 'Go to selected object' tool button functionality
            i) Create entity and find the entity in viewport
            ii) Change viewport position
            iii) Click on option and verify viewport position

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        editor_window = pyside_utils.get_editor_main_window()
        object_toolbar = editor_window.findChild(QtWidgets.QToolBar, "Object")

        def get_tool_button(option):
            button = pyside_utils.find_child_by_pattern(object_toolbar, option)
            if button.text() == option:
                return button

        # 1) Create new level with an Entity
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )
        general.idle_wait(3.0)

        # 2) Open the Object Toolbar
        if not object_toolbar.isVisible():
            object_toolbar.toggleViewAction().trigger()
        if object_toolbar.isVisible():
            print("Object tool bar opened successfully")

        # 3) Verify 'Go to selected object' tool button functionality
        # i)Create entity and find the entity in viewport
        editor.ToolsApplicationRequestBus(bus.Broadcast, "CreateNewEntity", EntityId.EntityId())

        # Find entity in viewport and get current position
        entity_outliner = pyside_utils.find_child_by_pattern(editor_window, "Entity Outliner (PREVIEW)")
        action = pyside_utils.find_child_by_pattern(entity_outliner, "Find in viewport")
        default_pos = general.get_current_view_position()
        action.trigger()
        self.wait_for_condition(lambda: default_pos != general.get_current_view_position(), 2.0)
        old_pos = general.get_current_view_position()

        # ii) Change viewport position
        general.set_current_view_position(old_pos.x + 10.0, old_pos.y + 10.0, old_pos.z + 10.0)
        self.wait_for_condition(lambda: old_pos != general.get_current_view_position(), 2.0)
        current_pos = general.get_current_view_position()

        # iii) Click on option and verify viewport position
        button = get_tool_button("Go to selected object")
        button.click()
        self.wait_for_condition(lambda: current_pos != general.get_current_view_position(), 2.0)
        new_pos = general.get_current_view_position()
        if new_pos != current_pos and new_pos == old_pos:
            print("Go to selected object tool button is responsive")


test = TestObjectToolbar()
test.run_test()
