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
C24064530 : Game Menu Options
https://testrail.agscollab.com/index.php?/cases/view/24064530

C16780793: Game Menu Options (New Viewport Interaction Model)
https://testrail.agscollab.com/index.php?/cases/view/16780793
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import Tests.ly_shared.pyside_utils as pyside_utils
from PySide2 import QtWidgets
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class TestGameMenuOptions(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="game_menu_options: ", args=["level"])

    def run_test(self):
        """
        Summary:
        Interact with Game Menu options and verify if all the options are working.

        Expected Behavior:
        The Game menu functions normally.

        Test Steps:
         1) Open level
         2) Interact with Game Menu options

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """
        game_menu_options = [
            ("Play Game",),
            ("Simulate",),
            ("Export to Engine",),
            ("Export Occlusion Mesh",),
            ("Enable Camera Terrain Collision",),
            ("Move Player and Camera Separately",),
            ("AI", "Request a full MNM rebuild",),
            ("AI", "Show Navigation Areas",),
            ("AI", "Continuous Update",),
            ("AI", "Visualize Navigation Accessibility",),
            ("AI", "View Agent Type",),
            ("Audio", "Stop All Sounds",),
            ("Audio", "Refresh Audio",),
            ("Debugging", "Configure ToolBox Macros",),
            ("Debugging", "ToolBox Macros",),
            ("Debugging", "Error Report",),
        ]

        def on_focus_changed(old, new):
            print("Focus Changed")
            QtWidgets.QApplication.activeModalWidget().close()

        def on_action_triggered(action_name):
            print(f"{action_name} Action triggered")
            if action_name == "Simulate":
                general.exit_simulation_mode()
            elif action_name == "Play Game":
                general.exit_game_mode()

        # 1) Create a new level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )
        general.idle_wait(3.0)

        # 2) Interact with Game Menu options
        try:
            editor_window = pyside_utils.get_editor_main_window()
            app = QtWidgets.QApplication.instance()
            app.focusChanged.connect(on_focus_changed)
            for option in game_menu_options:
                action = pyside_utils.get_action_for_menu_path(editor_window, "Game", *option)
                if action.isVisible():
                    print(f"{option} is visible in Game Menu")
                    trig_func = lambda: on_action_triggered(action.iconText())
                    action.triggered.connect(trig_func)
                    action.trigger()
                    action.triggered.disconnect(trig_func)
                    general.idle_wait(2.0)
                else:
                    print(f"{option} is not visible in Game Menu")
        except Exception as e:
            print(e)
        finally:
            app.focusChanged.disconnect(on_focus_changed)

test = TestGameMenuOptions()
test.run()
