"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Object to house all the Qt Objects used when testing and manipulating the O3DE UI
"""
import pyside_utils
import editor_python_test_tools.QtPyScriptCanvasEditor as wrapper_sc_editor
import editor_python_test_tools.QtPyScriptCanvasVariableManager as wapper_variable_manager


class QtPyWidgetContainer:
    """
    Container class for QtPy objects used in testing/manipulating the UI

    """

    def __init__(self):
        self.editor_main_window = pyside_utils.get_editor_main_window()
        self.wrapper_sc_editor = None
        self.sc_editor_main_window = None
        self.variable_manager = None

    def initialize_SC_objects(self):

        self.wrapper_sc_editor = wrapper_sc_editor.QtPyScriptCanvasEditor(self.editor_main_window)
        self.sc_editor_main_window = self.wrapper_sc_editor.get_sc_main_pane()

    def initialize_variable_manager(self):

        self.variable_manager = wapper_variable_manager.QtPyScriptCanvasVariableManager(self.wrapper_sc_editor.sc_editor)

    def get_SC_editor_wrapper(self):

        return self.wrapper_sc_editor

    def get_variable_manager(self):

        return self.variable_manager

