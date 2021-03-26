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
C6317020: Basic UI Scaling
https://testrail.agscollab.com/index.php?/cases/view/6317020
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import Tests.ly_shared.pyside_utils as pyside_utils
from PySide2 import QtWidgets
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class TestBasicUIScalingFunction(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="basic_ui_scaling: ", args=["level"])

    def run_test(self):
        """
        Summary:
        Resize the editor window and verify if the editor scaling works.

        Expected Behavior:
        The editor window/docked widgets is scalable/resizable into windowed mode.

        Test Steps:
        1) Resize the editor window couple of times
        2) Resize the docked widget

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def check_size(widget, widget_name, width, height):
            if widget.geometry().width() == width and widget.geometry().height() == height:
                print(f"{widget_name} size is {width} X {height}")
            else:
                print("Widget size not equal to expected value")

        # 1) Resize the editor window couple of times
        # wait util the editor opens
        general.idle_wait(3.0)
        editor_window = pyside_utils.get_editor_main_window()
        editor_window.resize(1000, 500)
        check_size(editor_window, "Editor", 1000, 500)

        editor_window.resize(2000, 1000)
        check_size(editor_window, "Editor", 2000, 1000)

        # 2) Resize the docked widget
        main_window = editor_window.findChild(QtWidgets.QMainWindow)
        asset_browser = main_window.findChild(QtWidgets.QDockWidget, "Asset Browser")
        asset_browser.resize(100, 100)
        check_size(asset_browser, "Docked Widget", 100, 100)


test = TestBasicUIScalingFunction()
test.run()
