"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

"""
C24064529: Base Edit Menu Options
"""

import os
import sys

import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
from editor_python_test_tools.editor_test_helper import EditorTestHelper
import editor_python_test_tools.pyside_utils as pyside_utils


class TestEditMenuOptions(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="Menus_EditMenuOptions", args=["level"])

    def run_test(self):
        """
        Summary:
        Interact with Edit Menu options and verify if all the options are working.

        Expected Behavior:
        The Edit menu functions normally.

        Test Steps:
         1) Create a temp level
         2) Interact with Edit Menu options

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """
        edit_menu_options = [
            ("Undo",),
            ("Redo",),
            ("Duplicate",),
            ("Delete",),
            ("Select All",),
            ("Invert Selection",),
            ("Toggle Pivot Location",),
            ("Reset Entity Transform",),
            ("Reset Manipulator",),
            ("Reset Transform (Local)",),
            ("Reset Transform (World)",),
            ("Hide Selection",),
            ("Show All",),
            ("Modify", "Snap", "Snap angle"),
            ("Modify", "Transform Mode", "Move"),
            ("Modify", "Transform Mode", "Rotate"),
            ("Modify", "Transform Mode", "Scale"),
            ("Editor Settings", "Global Preferences"),
            ("Editor Settings", "Graphics Settings"),
            ("Editor Settings", "Editor Settings Manager"),
            ("Editor Settings", "Graphics Performance", "PC", "Very High"),
            ("Editor Settings", "Graphics Performance", "PC", "High"),
            ("Editor Settings", "Graphics Performance", "PC", "Medium"),
            ("Editor Settings", "Graphics Performance", "PC", "Low"),
            ("Editor Settings", "Graphics Performance", "OSX Metal", "Very High"),
            ("Editor Settings", "Graphics Performance", "OSX Metal", "High"),
            ("Editor Settings", "Graphics Performance", "OSX Metal", "Medium"),
            ("Editor Settings", "Graphics Performance", "OSX Metal", "Low"),
            ("Editor Settings", "Graphics Performance", "Android", "Very High"),
            ("Editor Settings", "Graphics Performance", "Android", "High"),
            ("Editor Settings", "Graphics Performance", "Android", "Medium"),
            ("Editor Settings", "Graphics Performance", "Android", "Low"),
            ("Editor Settings", "Graphics Performance", "iOS", "Very High"),
            ("Editor Settings", "Graphics Performance", "iOS", "High"),
            ("Editor Settings", "Graphics Performance", "iOS", "Medium"),
            ("Editor Settings", "Graphics Performance", "iOS", "Low"),
            ("Editor Settings", "Keyboard Customization", "Customize Keyboard"),
            ("Editor Settings", "Keyboard Customization", "Export Keyboard Settings"),
            ("Editor Settings", "Keyboard Customization", "Import Keyboard Settings"),
        ]

        # 1) Create and open the temp level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=False,
        )

        def on_action_triggered(action_name):
            print(f"{action_name} Action triggered")

        # 2) Interact with Edit Menu options
        try:
            editor_window = pyside_utils.get_editor_main_window()
            for option in edit_menu_options:
                action = pyside_utils.get_action_for_menu_path(editor_window, "Edit", *option)
                trig_func = lambda: on_action_triggered(action.iconText())
                action.triggered.connect(trig_func)
                action.trigger()
                action.triggered.disconnect(trig_func)
        except Exception as e:
            self.test_success = False
            print(e)


test = TestEditMenuOptions()
test.run()
