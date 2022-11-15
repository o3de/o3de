"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Object to house all the Qt Objects used when testing and manipulating the Script Canvas editor and its tools
"""
from PySide2 import QtWidgets
import pyside_utils
from editor_python_test_tools.QtPyCommon import QtPyCommon
from editor_python_test_tools.QtPyScriptCanvasVariableManager import QtPyScriptCanvasVariableManager
from editor_python_test_tools.QtPyScriptCanvasNodePalette import QtPyScriptCanvasNodePalette
from editor_python_test_tools.QtPyScriptCanvasNodeInspector import QtPyScriptCanvasNodeInspector
from consts.scripting import (SCRIPT_CANVAS_UI)


class QtPyScriptCanvasEditor(QtPyCommon):
    """
    QtPy class for handling the behavior of the Script Canvas editor. Contains references to other qtpy objects
    reliant on the script canvas editor

    Note: This class contains a reference to the SC editor's main window AND the SC editor itself. This was done because
    the other QtPy elements such as the SC variable manager can be toggled on and off by the user but the main pane
    cannot. If a need to separate the two objects is discovered the main pane object will be put into its own QtPy class
    """

    def __init__(self, editor_main_window: QtWidgets.QMainWindow):
        super().__init__()
        self.sc_editor = editor_main_window.findChild(QtWidgets.QDockWidget, SCRIPT_CANVAS_UI)
        self.sc_editor_main_pane = self.sc_editor.findChild(QtWidgets.QMainWindow)
        self.variable_manager = QtPyScriptCanvasVariableManager(self)
        self.node_palette = QtPyScriptCanvasNodePalette(self)
        self.node_inspector = QtPyScriptCanvasNodeInspector(self)
        self.menu_bar = self.sc_editor.findChild(QtWidgets.QMenuBar)

    def trigger_undo_action(self) -> None:
        """
        function for commanding the sc editor to perform an undo action

        returns None
        """
        undo_redo_action = self.sc_editor.findChild(QtWidgets.QAction, "action_Undo")

        assert undo_redo_action is not None, "Unable to create undo action."

        undo_redo_action.trigger()

    def trigger_redo_action(self) -> None:
        """
        function for commanding the sc editor ot perform a redo action

        returns: None
        """
        undo_redo_action = self.sc_editor.findChild(QtWidgets.QAction, "action_Redo")

        assert undo_redo_action is not None, "Unable to create redo action."

        undo_redo_action.trigger()

    def create_new_script_canvas_graph(self) -> None:
        """
        Function for opening a new untitled script canvas graph file for edit

        returns: None
        """
        create_new_graph_action = pyside_utils.find_child_by_pattern(
            self.sc_editor_main_pane, {"objectName": "action_New_Script", "type": QtWidgets.QAction}
        )

        assert create_new_graph_action is not None, "Unable to create new script canvas graph action."

        create_new_graph_action.trigger()
