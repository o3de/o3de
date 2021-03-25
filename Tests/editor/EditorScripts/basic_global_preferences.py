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
C6321491: Basic Function: Global Preferences
https://testrail.agscollab.com/index.php?/cases/view/6321491
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import Tests.ly_shared.pyside_utils as pyside_utils
from PySide2 import QtWidgets
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class BasicGlobalPreferencesTest(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="basic_global_preferences: ")

    @pyside_utils.wrap_async
    async def run_test(self):
        """
        Summary:
        Verify if we can change some value in the Global Preferences in Editor Settings

        Expected Behavior:
        Global preferences can be changed and saved.

        Test Steps:
         1) Change Toolbar Icon size in Global Preferences
         2) Verify if the Toolbar Icon size changed
         3) Reset the Toolbar Icon Size

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        async def change_tool_button_size(size):
            action = pyside_utils.get_action_for_menu_path(
                editor_window, "Edit", "Editor Settings", "Global Preferences"
            )
            print(f"Global Preferences action triggered")
            pyside_utils.trigger_action_async(action)
            active_modal_widget = await pyside_utils.wait_for_modal_widget()
            combo_box = await pyside_utils.wait_for_child_by_hierarchy(
                active_modal_widget,
                "EditorPreferencesDialog",
                ...,
                "propertyEditor",
                ...,
                dict(type=QtWidgets.QFrame, text="Toolbar Icon Size"),
                ...,
                QtWidgets.QComboBox
            )
            combo_box.setCurrentIndex(combo_box.findText(size))
            button_box = active_modal_widget.findChild(QtWidgets.QDialogButtonBox, "buttonBox")
            button_box.button(QtWidgets.QDialogButtonBox.Ok).click()
            print(f"Toolbar Icon size set to {size}")
            await pyside_utils.wait_for_destroyed(active_modal_widget)

        # 1) Change Toolbar Icon size in Global Preferences
        editor_window = pyside_utils.get_editor_main_window()
        play_game_button = await pyside_utils.wait_for_child_by_hierarchy(
            editor_window,
            ...,
            "EditMode",
            ...,
            dict(type=QtWidgets.QToolButton, text="Play Game")
        )

        # First, make sure the toolbar icon size is the Default
        await change_tool_button_size("Default")
        initial_size = play_game_button.size()

        await change_tool_button_size("Large")
        def check_size():
            current_size = play_game_button.size()
            return initial_size.height() < current_size.height() and initial_size.width() < current_size.width()

        # 2) Verify if the Toolbar Icon size changed
        await pyside_utils.wait_for_condition(check_size)
        print("Toolbar Icon size changed in Global Preferences")

        # 3) Reset the Toolbar Icon Size
        await change_tool_button_size("Default")


test = BasicGlobalPreferencesTest()
test.run()
