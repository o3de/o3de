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
C15167491: Export a level
https://testrail.agscollab.com/index.php?/cases/view/15167491
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import azlmbr.editor as editor
import Tests.ly_shared.pyside_utils as pyside_utils
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class TestExportALevel(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="export_level: ", args=["level"])

    def run_test(self):
        """
        Summary:
        Export a Level and verify if the level file is created.

        Expected Behavior:
        The level is exported and the message "Export to the game was successfully done." appears in the console.
        The level.pak file is created after exporting.

        Test Steps:
        1) Create new level
        2) Export level

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def on_action_triggered():
            print("Game->Export to Engine Action triggered")

        # 1) Create new level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )
        general.idle_wait(3.0)

        # 2) Export level
        editor_window = pyside_utils.get_editor_main_window()
        action = pyside_utils.get_action_for_menu_path(editor_window, "Game", "Export to Engine")
        action.triggered.connect(on_action_triggered)
        action.trigger()
        action.triggered.disconnect(on_action_triggered)
        level_pak_file = os.path.join(
            "SamplesProject", "Levels", self.args["level"], "level.pak"
        )
        general.idle_wait(3.0)
        if os.path.exists(level_pak_file):
            print("level.pak file exists")


test = TestExportALevel()
test.run()
