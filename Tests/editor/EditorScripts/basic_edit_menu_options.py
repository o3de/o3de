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
C24064529: Base Edit Menu Options
https://testrail.agscollab.com/index.php?/cases/view/24064529
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import Tests.ly_shared.pyside_utils as pyside_utils
import Tests.ly_shared.hydra_editor_utils as hydra
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class TestEditMenuOptions(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="edit_menu_options", args=["level"])

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
            ("Hide Selection",),
            ("Show Selection",),
            ("Show Last Hidden",),
            ("Unhide All",),
            ("Modify", "Parent"),
            ("Modify", "Un-Parent"),
            ("Modify", "Align", "Align to grid"),
            ("Modify", "Align", "Align to object"),
            ("Modify", "Align", "Align object to surface"),
            ("Modify", "Constrain", "Constrain to X axis"),
            ("Modify", "Constrain", "Constrain to Y axis"),
            ("Modify", "Constrain", "Constrain to Z axis"),
            ("Modify", "Constrain", "Constrain to XY plane"),
            ("Modify", "Constrain", "Constrain to terrain/geometry"),
            ("Modify", "Snap", "Snap To Grid"),
            ("Modify", "Snap", "Snap angle"),
            ("Modify", "Fast Rotate", "Rotate X Axis"),
            ("Modify", "Fast Rotate", "Rotate Y Axis"),
            ("Modify", "Fast Rotate", "Rotate Z Axis"),
            ("Modify", "Fast Rotate", "Rotate Angle"),
            ("Modify", "Transform Mode", "Select mode"),
            ("Modify", "Transform Mode", "Move"),
            ("Modify", "Transform Mode", "Rotate"),
            ("Modify", "Transform Mode", "Scale"),
            ("Modify", "Transform Mode", "Select terrain"),
            ("Lock selection",),
            ("Unlock all",),
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
            ("Editor Settings", "Graphics Performance", "Apple TV"),
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
            use_terrain=True,
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
            print(e)

test = TestEditMenuOptions()
test.run()