"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Object to house all the Qt Objects used when testing and manipulating the Script Canvas editor and its tools
"""
from PySide2 import QtWidgets
import editor_python_test_tools.QtPyCommon as QtPyCommon
import pyside_utils
import editor_python_test_tools.QtPyScriptCanvasVariableManager as QtPyScriptCanvasVariableManager
from consts.scripting import (SCRIPT_CANVAS_UI)


class QtPyScriptCanvasEditor():
    """
    QtPy class for handling the behavior of the Script Canvas editor. Contains references to other qtpy objects
    reliant on the script canvas editor

    Note: This class contains a reference to the SC editor's main window AND the SC editor itself. This was done because
    the other QtPy elements such as the SC variable manager can be toggled on and off by the user but the main pane
    cannot. If a need to separate the two objects is discovered the main pane object will be put into its own QtPy class
    """

    def __init__(self, editor_main_window):

        self.sc_editor = editor_main_window.findChild(QtWidgets.QDockWidget, SCRIPT_CANVAS_UI)
        self.sc_editor_main_pane = self.sc_editor.findChild(QtWidgets.QMainWindow)
        self.variable_manager = QtPyScriptCanvasVariableManager.QtPyScriptCanvasVariableManager(self)

    def trigger_undo_action(self):
        """
        function for commanding the sc editor to perform an undo action

        returns None
        """
        undo_redo_action = self.sc_editor.findChild(QtWidgets.QAction, "action_Undo")
        undo_redo_action.trigger()

    def trigger_redo_action(self):
        """
        function for commanding the sc editor ot perform a redo action
        """
        undo_redo_action = self.sc_editor.findChild(QtWidgets.QAction, "action_Redo")
        undo_redo_action.trigger()

    def create_new_script_canvas_graph(self):
        """
        function for opening a new untitled script canvas graph for edit
        """
        create_new_graph_action = pyside_utils.find_child_by_pattern(
            self.sc_editor_main_pane, {"objectName": "action_New_Script", "type": QtWidgets.QAction}
        )
        create_new_graph_action.trigger()
