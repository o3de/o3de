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
C24064534: The View menu options function normally
https://testrail.agscollab.com/index.php?/cases/view/24064534
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import Tests.ly_shared.pyside_utils as pyside_utils
from PySide2 import QtWidgets
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class TestViewMenuOptions(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="view_menu_options: ", args=["level"])

    def run_test(self):
        """
        Summary:
        Interact with View Menu options and verify if all the options are working.

        Expected Behavior:
        The View menu functions normally.

        Test Steps:
         1) Interact with View Menu options

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """
        view_menu_options = [
            ("Center on Selection",),
            ("Show Quick Access Bar",),
            ("Layouts", "Default Layout"),
            ("Layouts", "User Legacy Layout"),
            ("Layouts", "Default Layout"),
            ("Layouts", "Save Layout"),
            ("Layouts", "Restore Default Layout"),
            ("Viewport", "Wireframe"),
            ("Viewport", "Ruler"),
            ("Viewport", "Grid Settings"),
            ("Viewport", "Configure Layout"),
            ("Viewport", "Goto Coordinates"),
            ("Viewport", "Center on Selection"),
            ("Viewport", "Goto Location"),
            ("Viewport", "Remember Location"),
            ("Viewport", "Change Move Speed"),
            ("Viewport", "Switch Camera"),
            ("Viewport", "Show/Hide Helpers"),
            ("Refresh Style",),
        ]

        general.idle_wait(3.0)

        def on_focus_changed(old, new):
            print("Focus Changed")
            QtWidgets.QApplication.activeModalWidget().close()

        def on_action_triggered(action_name):
            print(f"{action_name} Action triggered")

        # 1) Interact with View Menu options
        try:
            editor_window = pyside_utils.get_editor_main_window()
            app = QtWidgets.QApplication.instance()
            app.focusChanged.connect(on_focus_changed)
            for option in view_menu_options:
                action = pyside_utils.get_action_for_menu_path(editor_window, "View", *option)
                trig_func = lambda: on_action_triggered(action.iconText())
                action.triggered.connect(trig_func)
                action.trigger()
                action.triggered.disconnect(trig_func)
        except Exception as e:
            print(e)
        finally:
            app.focusChanged.disconnect(on_focus_changed)

test = TestViewMenuOptions()
test.run()
