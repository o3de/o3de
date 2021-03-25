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
C24064528: The File menu options function normally
https://testrail.agscollab.com/index.php?/cases/view/24064528
C16780778: The File menu options function normally-New view interaction Model enabled
https://testrail.agscollab.com/index.php?/cases/view/16780778
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import Tests.ly_shared.pyside_utils as pyside_utils
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class TestFileMenuOptions(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="file_menu_options: ", args=["level"])

    def run_test(self):
        """
        Summary:
        Interact with File Menu options and verify if all the options are working.

        Expected Behavior:
        The File menu functions normally.

        Test Steps:
         1) Open level
         2) Interact with File Menu options

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """
        file_menu_options = [
            ("New Level",),
            ("Open Level",),
            ("Import",),
            ("Save",),
            ("Save As",),
            ("Save Level Statistics",),
            ("Project Settings", "Switch Projects"),
            ("Project Settings", "Configure Gems"),
            ("Show Log File",),
            ("Resave All Slices",),
            ("Upgrade Legacy Entities",),
        ]

        # 1) Open level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )

        def on_action_triggered(action_name):
            print(f"{action_name} Action triggered")

        # 2) Interact with File Menu options
        try:
            editor_window = pyside_utils.get_editor_main_window()
            for option in file_menu_options:
                action = pyside_utils.get_action_for_menu_path(editor_window, "File", *option)
                trig_func = lambda: on_action_triggered(action.iconText())
                action.triggered.connect(trig_func)
                action.trigger()
                action.triggered.disconnect(trig_func)
        except Exception as e:
            print(e)


test = TestFileMenuOptions()
test.run()
