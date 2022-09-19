"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

"""
import pyside_utils
import azlmbr.legacy.general as general
import editor_python_test_tools.QtPyScriptCanvasEditor as QtPyScriptCanvasEditor
import editor_python_test_tools.QtPyScriptCanvasVariableManager as QtPyScriptCanvasVariableManager
from editor_python_test_tools.utils import TestHelper
from editor_python_test_tools.utils import Report
from consts.scripting import (SCRIPT_CANVAS_UI)
from consts.general import (WAIT_TIME_3)

class Tests():
    script_canvas_editor_opened = ("Script Canvas Editor opened successfully", "Failed to open Script Canvas Editor")
    script_canvas_editor_closed = ("Script Canvas Editor closed successfully", "Failed to close Script Canvas Editor")

class QtPyWidgetContainer:
    """
    Container class for QtPy objects used in testing/manipulating the UI

    """

    def __init__(self):
        self.editor_main_window = pyside_utils.get_editor_main_window()
        self.sc_editor = None
        self.variable_manager = None

    def open_script_canvas(self):
        """
        Function for instantiating the two core script canvas QtPy objects used in SC testing.
        sc_editor is the editor itself, which the other objects are anchored or embedded in
        sc_editor_main_window is the main pane that houses the graph UI and basic controls

        returns None
        """
        general.open_pane(SCRIPT_CANVAS_UI)
        result = TestHelper.wait_for_condition(lambda: general.is_pane_visible(SCRIPT_CANVAS_UI), WAIT_TIME_3)

        self.sc_editor = QtPyScriptCanvasEditor.QtPyScriptCanvasEditor(self.editor_main_window)
        self.variable_manager = QtPyScriptCanvasVariableManager.QtPyScriptCanvasVariableManager(
            self.sc_editor.sc_editor)

        Report.result(Tests.script_canvas_editor_opened, result is True)

    def close_script_canvas(self):

        general.close_pane(SCRIPT_CANVAS_UI)
        result = TestHelper.wait_for_condition(lambda: general.is_pane_visible(SCRIPT_CANVAS_UI) is False, WAIT_TIME_3)

        Report.result(Tests.script_canvas_editor_closed, result is True)
