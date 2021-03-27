"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

C6351301: Basic Function: Toolbars
https://testrail.agscollab.com/index.php?/cases/view/6351301
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as EntityId
import Tests.ly_shared.pyside_utils as pyside_utils
from PySide2 import QtWidgets
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class TestBasicToolBarFunction(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="basic_toolbar_function: ", args=["level"])

    def run_test(self):
        """
        Summary:
        Enable all menu toolbars and click between options in each of the toolbars and then disable all the toolbars.

        Expected Behavior:
        All menu toolbars can be enabled and disbled with each toolbar is functional and options can be selected.

        Test Steps:
        1) Create new level with an Entity
        2) Enable each of the toolbars in the list.
        3) Verify options from toolbars can be selected
            3.1) "Play Game" option from EditMode toolbar
            3.2) "Go to selected object" option from Object toolbar
        4) Disable each of the toolbars in the list.

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """
        editor_window = pyside_utils.get_editor_main_window()

        def fetch_vector3_parts(vec3):
            x = vec3.get_property("x")
            y = vec3.get_property("y")
            z = vec3.get_property("z")
            return (x, y, z)

        def enable_disable_toolbar(toolbar, enable=True):
            toolbar = editor_window.findChild(QtWidgets.QToolBar, i)
            action = toolbar.toggleViewAction()
            if enable:
                # Enable the toolbar
                if not toolbar.isVisible():
                    action.trigger()
                if toolbar.isVisible():
                    print(f"{i} toolbar is enabled")

            else:
                # Disable the toolbar
                if toolbar.isVisible():
                    action.trigger()
                if not toolbar.isVisible():
                    print(f"{i} toolbar is disbaled")

        menu_toolbar_list = [
            "EditMode",
            "Object",
            "debugViewsToolbar",
            "environmentModesToolbar",
            "viewModesToolbar",
        ]

        # 1) Create new level with an Entity
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )
        general.idle_wait(3.0)
        entityId = editor.ToolsApplicationRequestBus(bus.Broadcast, "CreateNewEntity", EntityId.EntityId())

        # 2) Enable each of the toolbars in the list.
        for i in menu_toolbar_list:
            enable_disable_toolbar(i)

        # 3) Select options from toolbar
        # 3.1) "Play Game" option from EditMode toolbar
        editors_toolbar = editor_window.findChild(QtWidgets.QToolBar, "EditMode")
        for i in editors_toolbar.children():
            if isinstance(i, QtWidgets.QToolButton) and i.text() == "Play Game":
                play_game_button = i
                break
        # Click on the tool button to verify ToolBar functionality
        play_game_button.click()
        general.idle_wait(1.0)

        if general.is_in_game_mode():
            print("In game mode: Play game ToolButton is responsive")
            general.exit_game_mode()

        # 3.2) "Go to selected object" option from Object toolbar
        position = fetch_vector3_parts(general.get_current_view_position())

        # Click on the tool button to verify ToolBar functionality
        object_toolbar = editor_window.findChild(QtWidgets.QToolBar, "Object")
        for i in object_toolbar.children():
            if isinstance(i, QtWidgets.QToolButton) and i.text() == "Go to selected object":
                general.select_object(editor.EditorEntityInfoRequestBus(bus.Event, "GetName", entityId))
                general.idle_wait(1.0)
                i.click()
                break
        general.idle_wait(1.0)
        new_pos = fetch_vector3_parts(general.get_current_view_position())
        if new_pos != position:
            print("Go to selected object option is responsive")

        # 4) Disable each of the toolbars in the list.
        for i in menu_toolbar_list:
            enable_disable_toolbar(i, False)


test = TestBasicToolBarFunction()
test.run()
