"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import azlmbr
import azlmbr.bus
import azlmbr.editor as editor

from PySide2 import QtWidgets
from shiboken2 import wrapInstance, getCppPointer


# The `view_pane_handlers` holds onto the callback handlers that get created
# for responding to requests for when the Editor needs to construct the view pane
view_pane_handlers = {}

# Helper method for registering a Python widget as a tool/view pane with the Editor
def register_view_pane(name, widget_type, options=editor.ViewPaneOptions()):
    global view_pane_handlers

    # The view pane names are unique in the Editor, so make sure one with the same name doesn't exist already
    if name in view_pane_handlers:
        return

    # This method will be invoked by the ViewPaneCallbackBus::CreateViewPaneWidget
    # when our view pane needs to be created
    def on_create_view_pane_widget(parameters):
        params = azlmbr.qt.QtForPythonRequestBus(azlmbr.bus.Broadcast, "GetQtBootstrapParameters")
        editor_id = QtWidgets.QWidget.find(params.mainWindowId)
        editor_main_window = wrapInstance(int(getCppPointer(editor_id)[0]), QtWidgets.QMainWindow)
        dock_main_window = editor_main_window.findChild(QtWidgets.QMainWindow)

        # Create the view pane widget parented to the Editor QMainWindow, so it can be found
        new_widget = widget_type(dock_main_window)

        return new_widget.winId()

    # Register our widget as an Editor view pane
    editor.EditorRequestBus(azlmbr.bus.Broadcast, 'RegisterCustomViewPane', name, 'Tools', options)

    # Connect to the ViewPaneCallbackBus in order to respond to requests to create our widget
    # We also need to store our handler so it will exist for the life of the Editor
    handler = azlmbr.bus.NotificationHandler("ViewPaneCallbackBus")
    handler.connect(name)
    handler.add_callback("CreateViewPaneWidget", on_create_view_pane_widget)
    view_pane_handlers[name] = handler

# Helper method for unregistering a Python widget as a tool/view pane with the Editor
def unregister_view_pane(name):
    global view_pane_handlers

    # No need to proceed if we don't have this view pane registered
    if name not in view_pane_handlers:
        return

    # Unregister our view pane from the Editor and remove our stored handler for it
    editor.EditorRequestBus(azlmbr.bus.Broadcast, 'UnregisterViewPane', name)
    del view_pane_handlers[name]
