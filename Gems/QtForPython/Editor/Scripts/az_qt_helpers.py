"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import azlmbr
import azlmbr.bus
import azlmbr.editor as editor

from PySide2 import QtWidgets
from shiboken2 import wrapInstance, getCppPointer


# The `view_pane_handlers` holds onto the callback handlers that get created
# for responding to requests for when the Editor needs to construct the view pane
view_pane_handlers = {}
registration_handlers = {}

# Helper method for retrieving the Editor QMainWindow instance
def get_editor_main_window():
    params = azlmbr.qt.QtForPythonRequestBus(azlmbr.bus.Broadcast, "GetQtBootstrapParameters")
    editor_id = QtWidgets.QWidget.find(params.mainWindowId)
    editor_main_window = wrapInstance(int(getCppPointer(editor_id)[0]), QtWidgets.QMainWindow)

    return editor_main_window

# Helper method for registering a Python widget as a tool/view pane with the Editor
def register_view_pane(name, widget_type, options=editor.ViewPaneOptions()):
    global view_pane_handlers

    # The view pane names are unique in the Editor, so make sure one with the same name doesn't exist already
    if name in view_pane_handlers:
        return

    # This method will be invoked by the ViewPaneCallbackBus::CreateViewPaneWidget
    # when our view pane needs to be created
    def on_create_view_pane_widget(parameters):
        editor_main_window = get_editor_main_window()
        dock_main_window = editor_main_window.findChild(QtWidgets.QMainWindow)

        # Create the view pane widget parented to the Editor QMainWindow, so it can be found
        new_widget = widget_type(dock_main_window)

        return new_widget.winId()

    def on_notify_register_views(parameters, my_name=name, my_options=options):
        # Register our widget as an Editor view pane
        print('Calling on_notify_register_views RegisterCustomViewPane')
        editor.EditorRequestBus(azlmbr.bus.Broadcast, 'RegisterCustomViewPane', my_name, 'Tools', my_options)

    # We keep a handler around in case a request for registering custom view panes comes later
    print('Initializing callback for RegisterCustomViewPane')
    registration_handler = azlmbr.bus.NotificationHandler("EditorEventBus")
    registration_handler.connect()
    registration_handler.add_callback("NotifyRegisterViews", on_notify_register_views)
    global registration_handlers
    registration_handlers[name] = registration_handler
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
