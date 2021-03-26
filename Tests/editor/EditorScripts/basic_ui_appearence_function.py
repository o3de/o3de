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
C6317014: Basic UI Appearance & Function
https://testrail.agscollab.com/index.php?/cases/view/6317014
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.legacy.general as general
import Tests.ly_shared.pyside_utils as pyside_utils
from PySide2 import QtWidgets, QtTest
from PySide2.QtCore import Qt
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class TestBasicUIAppearence(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="basic_ui_appearence_function: ", args=["level"])

    def run_test(self):
        """
        Summary:
        Open Lumberyard editor and check if basic UI components are loaded and responsive.

        Expected Behavior:
        Elements below of the UI are loaded and responsive.
            Menus
            Toolbars
            Tools
            Viewport

        Test Steps:
        1) Open level
        2) Verify if Tools are responsive - Opens some tool by triggering the Tool Menu actions and
           verifies if the tool has been opened and active.
        3) Verify if Menu items are responsive - Triggers the MenuBar actions of the Editor window.
        4) Verify if Viewport is responsive - Simulates right-click on the viewport to create a new Entity.
        5) Verify if Toolbars are responsive - Uses the "EditMode" toolbar to click on the Play Game
           ToolButton, and then verifies if the editor is in game mode.

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def on_action_triggered(action_name):
            print(f"{action_name} Action triggered")

        def on_action_trigger():
            print("Tools Action triggered")

        # 1) Open level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )

        editor_window = pyside_utils.get_editor_main_window()
        # 2) Verify if Tools are responsive
        tool_action = pyside_utils.get_action_for_menu_path(editor_window, "Tools", "Script Canvas")
        tool_action.triggered.connect(on_action_trigger)
        tool_action.trigger()
        tool_action.triggered.disconnect(on_action_trigger)
        # Verify if the tool is opened
        def is_script_canvas_active_window():
            tool = editor_window.findChild(QtWidgets.QWidget, "Script Canvas")
            if tool:
                return tool.isActiveWindow()

            return False
        if self.wait_for_condition(is_script_canvas_active_window, 2.0):
            print("Tool opened")

        # 3) Verify if Menu items are responsive
        menu_bar = editor_window.menuBar()
        for action in menu_bar.actions():
            trig_func = lambda: on_action_triggered(action.iconText())
            action.triggered.connect(trig_func)
            action.trigger()
            action.triggered.disconnect(trig_func)

        # 4) Verify if Viewport is responsive
        # Clear the current selection and create a new Entity via the viewport right-click menu
        render_overlay = editor_window.findChild(QtWidgets.QWidget, "renderOverlay")
        general.clear_selection()
        pyside_utils.trigger_context_menu_entry(render_overlay.parent(), "Create entity")
        selected_entities = editor.ToolsApplicationRequestBus(bus.Broadcast, 'GetSelectedEntities')
        if len(selected_entities) == 1:
            entity_id = selected_entities[0]
            name = editor.EditorEntityInfoRequestBus(bus.Event, "GetName", entity_id)
            if name == "Entity2":
                print("Viewport is responsive")

        # 5) Verify if Toolbars are responsive
        editors_toolbar = editor_window.findChild(QtWidgets.QToolBar, "EditMode")
        play_game_button = pyside_utils.find_child_by_pattern(editors_toolbar, "Play Game")

        # Click on the tool button to verify ToolBar functionality
        play_game_button.click()

        if self.wait_for_condition(lambda: general.is_in_game_mode()):
            print("In game mode: Play game ToolButton is responsive")
        general.exit_game_mode()


test = TestBasicUIAppearence()
test.run()
