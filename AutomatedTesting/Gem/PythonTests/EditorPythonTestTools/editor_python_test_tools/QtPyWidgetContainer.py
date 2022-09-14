"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Object to house all the Qt Objects used when testing and manipulating the O3DE UI
"""
import pyside_utils
import editor_python_test_tools.QtPyScriptCanvasEditor as QtPyScriptCanvasEditor
import editor_python_test_tools.QtPyScriptCanvasVariableManager as QtPyScriptCanvasVariableManager


class QtPyWidgetContainer:
    """
    Container class for QtPy objects used in testing/manipulating the UI

    """

    def __init__(self):
        self.editor_main_window = pyside_utils.get_editor_main_window()
        self.sc_editor = None
        self.variable_manager = None

    def initialize_SC_objects(self):
        """
        Function for instantiating the two core script canvas QtPy objects used in SC testing.
        sc_editor is the editor itself, which the other objects are anchored or embedded in
        sc_editor_main_window is the main pane that houses the graph UI and basic controls

        returns None
        """
        self.sc_editor = QtPyScriptCanvasEditor.QtPyScriptCanvasEditor(self.editor_main_window)

    def initialize_variable_manager(self):
        """
        function for instantiating the variable manager tool's QtPy object.
        """
        self.variable_manager = QtPyScriptCanvasVariableManager.QtPyScriptCanvasVariableManager(self.sc_editor.sc_editor)

    def get_script_canvas_editor(self):
        """
        function for retrieving the sc editor's QtPy object
        """
        return self.sc_editor

    def get_variable_manager(self):
        """
        function for retrieving the variable manager's QtPy object
        """
        return self.variable_manager

