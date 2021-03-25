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
C6317035: Basic Viewport Configuration
https://testrail.agscollab.com/index.php?/cases/view/6317035
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import Tests.ly_shared.pyside_utils as pyside_utils
from PySide2 import QtWidgets
from PySide2.QtTest import QTest
from PySide2.QtCore import Qt
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class BasicViewportConfigurationTest(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="basic_viewport_configuration", args=["level"])

    @pyside_utils.wrap_async
    async def run_test(self):
        """
        Summary:
        Verify id the basic viewport configuration works as expected.

        Expected Behavior:
        1) Layout Configuration dialog opens.
        2) Layout configuration can be changed.
        3) All viewports function properly and independently of one another.
        4) The viewport is reverted to a single, large window.

        Test Steps:
        1) Open a new level
        2) Configure the viewport layer to a different layout
        3) Verify if viewport 1 is functional
        4) Verify if viewport 2 is functional
        5) Reset the layout back to original

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def verify_viewport_function(viewport_main_window_object):
            """
            To check if the viewport camera controls are working, we can use the key press 'Left Arrow'
            which manuallly would have moved the camera to its left. So by simulating the Left Arrow press and
            comparing the current view position before and after the key press, we can verify if the view has indeed
            been modified and there by confirming if viewport is active.
            """
            viewport = viewport_main_window_object.findChildren(QtWidgets.QWidget, "renderOverlay")[0].parent()
            initial_x = general.get_current_view_position().x
            viewport.setFocus()
            QTest.keyPress(viewport, Qt.Key_Left, Qt.NoModifier)
            general.idle_wait(0.5)
            QTest.keyRelease(viewport, Qt.Key_Left, Qt.NoModifier, delay=2)
            current_x = general.get_current_view_position().x
            return current_x < initial_x

        async def select_layout(layout_index):
            action = pyside_utils.get_action_for_menu_path(editor_window, "View", "Viewport", "Configure Layout")
            pyside_utils.trigger_action_async(action)
            active_modal_widget = await pyside_utils.wait_for_modal_widget()
            layout_config_dialog = active_modal_widget.findChild(QtWidgets.QDialog, "CLayoutConfigDialog")
            list_view = layout_config_dialog.findChild(QtWidgets.QListView, "m_layouts")
            pyside_utils.item_view_mouse_click(list_view, layout_index)
            button_box = layout_config_dialog.findChild(QtWidgets.QDialogButtonBox, "m_buttonBox")
            button_box.button(QtWidgets.QDialogButtonBox.Ok).click()        

            print(f"Layout set to {layout_index}")

        # 1) Open level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )

        editor_window = pyside_utils.get_editor_main_window()

        # 2) Configure the viewport layer to a different layout
        # In this case layout with 2 viewports placed side by side is selected
        await select_layout(1)
        main_window = editor_window.findChild(QtWidgets.QMainWindow).findChild(QtWidgets.QMainWindow)
        central_widget = main_window.centralWidget()
        await pyside_utils.wait_for_condition(lambda: central_widget.findChild(QtWidgets.QSplitter) is not None)
        splitter = central_widget.findChild(QtWidgets.QSplitter)

        # 3) Verify if viewport 1 is functional
        await pyside_utils.wait_for_condition(lambda: verify_viewport_function(splitter.children()[0]), 3.0)
        print(f"Viewport 1 camera controls are functional: {verify_viewport_function(splitter.children()[0])}")

        # 4) Verify if viewport 2 is functional
        await pyside_utils.wait_for_condition(lambda: verify_viewport_function(splitter.children()[1]), 3.0)
        print(f"Viewport 2 camera controls are functional: {verify_viewport_function(splitter.children()[1])}")

        # 5) Reset the layout back to original
        await select_layout(0) 

test = BasicViewportConfigurationTest()
test.run()