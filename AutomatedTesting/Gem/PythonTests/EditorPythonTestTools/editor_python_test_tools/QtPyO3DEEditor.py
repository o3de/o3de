"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Object to house all the Qt Objects used when testing and manipulating the O3DE UI
"""
import pyside_utils
import editor_python_test_tools.QtPyCommon as QtPyCommon
import azlmbr.legacy.general as general
import editor_python_test_tools.QtPyScriptCanvasEditor as QtPyScriptCanvasEditor
from editor_python_test_tools.utils import TestHelper
from editor_python_test_tools.utils import Report
from consts.scripting import (SCRIPT_CANVAS_UI)
from consts.general import (WAIT_TIME_3)

class Tests():
    script_canvas_editor_opened = ("Script Canvas Editor opened successfully", "Failed to open Script Canvas Editor")
    script_canvas_editor_closed = ("Script Canvas Editor closed successfully", "Failed to close Script Canvas Editor")


class QtPyO3DEEditor(QtPyCommon.QtPyCommon):
    """
    Container class for QtPy objects used in testing/manipulating the UI

    """

    def __init__(self):
        super().__init__()
        self.editor_main_window = pyside_utils.get_editor_main_window()
        self.sc_editor = None

    def open_script_canvas(self):
        """
        Function for instantiating the core script canvas QtPy object and its associated other script canvas tools.

        returns reference to the QtPy script canvas editor object
        """
        general.open_pane(SCRIPT_CANVAS_UI)
        result = TestHelper.wait_for_condition(lambda: general.is_pane_visible(SCRIPT_CANVAS_UI), WAIT_TIME_3)

        self.sc_editor = QtPyScriptCanvasEditor.QtPyScriptCanvasEditor(self.editor_main_window)

        Report.result(Tests.script_canvas_editor_opened, result is True)

        return self.sc_editor

    def close_script_canvas(self):
        """
        this function will close the script canvas UI. It will not clear the reference to the sc_editor
        """

        general.close_pane(SCRIPT_CANVAS_UI)
        result = TestHelper.wait_for_condition(lambda: general.is_pane_visible(SCRIPT_CANVAS_UI) is False, WAIT_TIME_3)

        Report.result(Tests.script_canvas_editor_closed, result is True)
