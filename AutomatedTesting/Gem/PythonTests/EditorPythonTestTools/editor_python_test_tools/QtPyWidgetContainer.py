"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

"""

import editor_python_test_tools.QtPyO3DEEditor as QtPyO3DEEditor

class Tests():
    script_canvas_editor_opened = ("Script Canvas Editor opened successfully", "Failed to open Script Canvas Editor")
    script_canvas_editor_closed = ("Script Canvas Editor closed successfully", "Failed to close Script Canvas Editor")

class QtPyWidgetContainer:
    """
    Container class for QtPy objects used in testing/manipulating the UI

    """

    def __init__(self):
        self.editor_main_window = QtPyO3DEEditor.QtPyO3DEEditor()
        self.sc_editor = None
        self.variable_manager = None
