"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
from PySide2 import QtWidgets
from consts.scripting import (SCRIPT_CANVAS_UI)


class QtPyScriptCanvasEditor:
    """
    QtPy class for handling the behavior of the Script Canvas editor.

    Note: This class contains a reference to the SC editor's main window AND the SC editor itself. This was done because the other
    QtPy elements such as the SC variable manager can be toggled on and off by the user but the main pane cannot. If a need
    to separate the two objects is discovered the main pane object will be put into its own QtPy class
    """

    def __init__(self, editor_main_window):
        self.sc_editor = editor_main_window.findChild(QtWidgets.QDockWidget, SCRIPT_CANVAS_UI)
        self.sc_editor_main_window = self.sc_editor.findChild(QtWidgets.QMainWindow)

    def get_main_pane(self):
        """
        function for retrieving the sc_editor's main pane QtPy object
        """
        return self.sc_editor_main_window

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


