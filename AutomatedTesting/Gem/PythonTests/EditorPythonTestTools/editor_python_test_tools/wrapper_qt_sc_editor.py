"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Object to house all the Qt Objects used when testing and manipulating the O3DE UI
"""
from PySide2 import QtWidgets
from scripting_utils.scripting_constants import (SCRIPT_CANVAS_UI)

class WrapperQTSCEditor:

    def __init__(self, editor_main_window):
        self.sc_editor = editor_main_window.findChild(QtWidgets.QDockWidget, SCRIPT_CANVAS_UI)
        self.sc_editor_main_window = self.sc_editor.findChild(QtWidgets.QMainWindow)

    def get_sc_main_pane(self):
        return self.sc_editor_main_window

    def trigger_undo_action(self):
        undo_redo_action = self.sc_editor.findChild(QtWidgets.QAction, "action_Undo")
        undo_redo_action.trigger()

    def trigger_redo_action(self):
        undo_redo_action = self.sc_editor.findChild(QtWidgets.QAction, "action_Redo")
        undo_redo_action.trigger()

