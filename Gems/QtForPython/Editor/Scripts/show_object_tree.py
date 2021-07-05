"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import azlmbr
from shiboken2 import wrapInstance, getCppPointer
from PySide2 import QtCore, QtWidgets, QtGui
from PySide2.QtCore import QEvent, Qt
from PySide2.QtWidgets import QAction, QDialog, QHeaderView, QLabel, QLineEdit, QPushButton, QSplitter, QTreeWidget, QTreeWidgetItem, QWidget, QAbstractButton

class InspectPopup(QWidget):
    def __init__(self, parent=None):
        super(InspectPopup, self).__init__(parent)

        self.setWindowFlags(Qt.Popup)

        object_name = "InspectPopup"
        self.setObjectName(object_name)
        self.setStyleSheet("#{name} {{ background-color: #6441A4; }}".format(name=object_name))
        self.setContentsMargins(10, 10, 10, 10)

        layout = QtWidgets.QGridLayout()
        self.name_label = QLabel("Name:")
        self.name_value = QLabel("")
        layout.addWidget(self.name_label, 0, 0)
        layout.addWidget(self.name_value, 0, 1)

        self.type_label = QLabel("Type:")
        self.type_value = QLabel("")
        layout.addWidget(self.type_label, 1, 0)
        layout.addWidget(self.type_value, 1, 1)

        self.geometry_label = QLabel("Geometry:")
        self.geometry_value = QLabel("")
        layout.addWidget(self.geometry_label, 2, 0)
        layout.addWidget(self.geometry_value, 2, 1)

        self.setLayout(layout)

    def update_widget(self, new_widget):
        name = "(None)"
        type_str = "(Unknown)"
        geometry_str = "(Unknown)"
        if new_widget:
            type_str = str(type(new_widget))

            geometry_rect = new_widget.geometry()
            geometry_str = "x: {x}, y: {y}, width: {width}, height: {height}".format(x=geometry_rect.x(), y=geometry_rect.y(), width=geometry_rect.width(), height=geometry_rect.height())

            # Not all of our widgets have their objectName set
            if new_widget.objectName():
                name = new_widget.objectName()

        self.name_value.setText(name)
        self.type_value.setText(type_str)
        self.geometry_value.setText(geometry_str)

class ObjectTreeDialog(QDialog):
    def __init__(self, parent=None, root_object=None):
        super(ObjectTreeDialog, self).__init__(parent)
        self.setWindowTitle("Object Tree")

        layout = QtWidgets.QVBoxLayout()

        # Tree widget for displaying our object hierarchy
        self.tree_widget = QTreeWidget()
        self.tree_widget_columns = [
            "TYPE",
            "OBJECT NAME",
            "TEXT",
            "ICONTEXT",
            "TITLE",
            "WINDOW_TITLE",
            "CLASSES",
            "POINTER_ADDRESS",
            "GEOMETRY"
        ]
        self.tree_widget.setColumnCount(len(self.tree_widget_columns))
        self.tree_widget.setHeaderLabels(self.tree_widget_columns)

        # Only show our type and object name columns.  The others we only use to store data so that
        # we can use the built-in QTreeWidget.findItems to query.
        for column_name in self.tree_widget_columns:
            if column_name == "TYPE" or column_name == "OBJECT NAME":
                continue
                
            column_index = self.tree_widget_columns.index(column_name)
            self.tree_widget.setColumnHidden(column_index, True)

        header = self.tree_widget.header()
        header.setSectionResizeMode(0, QHeaderView.ResizeToContents)
        header.setSectionResizeMode(1, QHeaderView.ResizeToContents)

        # Populate our object tree widget
        # If a root object wasn't specified, then use the Editor main window
        if not root_object:
            params = azlmbr.qt.QtForPythonRequestBus(azlmbr.bus.Broadcast, "GetQtBootstrapParameters")
            editor_id = QtWidgets.QWidget.find(params.mainWindowId)
            editor_main_window = wrapInstance(int(getCppPointer(editor_id)[0]), QtWidgets.QMainWindow)
            root_object = editor_main_window
        self.build_tree(root_object, self.tree_widget)

        # Listen for when the tree widget selection changes so we can update
        # selected item properties
        self.tree_widget.itemSelectionChanged.connect(self.on_tree_widget_selection_changed)

        # Split our tree widget with a properties view for showing more information about
        # a selected item. We also use a stacked layout for the properties view so that
        # when nothing has been selected yet, we can show a message informing the user
        # that something needs to be selected.
        splitter = QSplitter()
        splitter.addWidget(self.tree_widget)
        self.widget_properties = QWidget(self)
        self.stacked_layout = QtWidgets.QStackedLayout()
        self.widget_info = QWidget()
        form_layout = QtWidgets.QFormLayout()
        self.name_value = QLineEdit("")
        self.name_value.setReadOnly(True)
        self.type_value = QLabel("")
        self.geometry_value = QLabel("")
        self.text_value = QLabel("")
        self.icon_text_value = QLabel("")
        self.title_value = QLabel("")
        self.window_title_value = QLabel("")
        self.classes_value = QLabel("")
        form_layout.addRow("Name:", self.name_value)
        form_layout.addRow("Type:", self.type_value)
        form_layout.addRow("Geometry:", self.geometry_value)
        form_layout.addRow("Text:", self.text_value)
        form_layout.addRow("Icon Text:", self.icon_text_value)
        form_layout.addRow("Title:", self.title_value)
        form_layout.addRow("Window Title:", self.window_title_value)
        form_layout.addRow("Classes:", self.classes_value)
        self.widget_info.setLayout(form_layout)

        self.widget_properties.setLayout(self.stacked_layout)
        self.stacked_layout.addWidget(QLabel("Select an object to view its properties"))
        self.stacked_layout.addWidget(self.widget_info)
        splitter.addWidget(self.widget_properties)

        # Give our splitter stretch factor of 1 so it will expand to take more room over
        # the footer
        layout.addWidget(splitter, 1)

        # Create our popup widget for showing information when hovering over widgets
        self.hovered_widget = None
        self.inspect_mode = False
        self.inspect_popup = InspectPopup()
        self.inspect_popup.resize(100, 50)
        self.inspect_popup.hide()

        # Add a footer with a button to switch to widget inspect mode
        self.footer = QWidget()
        footer_layout = QtWidgets.QHBoxLayout()
        self.inspect_button = QPushButton("Pick widget to inspect")
        self.inspect_button.clicked.connect(self.on_inspect_clicked)
        footer_layout.addStretch(1)
        footer_layout.addWidget(self.inspect_button)
        self.footer.setLayout(footer_layout)
        layout.addWidget(self.footer)

        self.setLayout(layout)

        # Delete ourselves when the dialog is closed, so that we don't stay living in the background
        # since we install an event filter on the application
        self.setAttribute(Qt.WA_DeleteOnClose, True)

        # Listen to events at the application level so we can know when the mouse is moving
        app = QtWidgets.QApplication.instance()
        app.installEventFilter(self)

    def eventFilter(self, obj, event):
        # Look for mouse movement events so we can see what widget the mouse is hovered over
        event_type = event.type()
        if event_type == QEvent.MouseMove:
            global_pos = event.globalPos()

            # Make our popup follow the mouse, but we need to offset it by 1, 1 otherwise
            # the QApplication.widgetAt will always return our popup instead of the Editor
            # widget since it is on top
            self.inspect_popup.move(global_pos + QtCore.QPoint(1, 1))

            # Find out which widget is under our current mouse position
            hovered_widget = QtWidgets.QApplication.widgetAt(global_pos)
            if self.hovered_widget:
                # Bail out, this is the same widget we are already hovered on
                if self.hovered_widget is hovered_widget:
                    return False

            # Update our hovered widget and label
            self.hovered_widget = hovered_widget
            self.update_hovered_widget_popup()
        elif event_type == QEvent.KeyRelease:
            if event.key() == Qt.Key_Escape:
                # Cancel the inspect mode if the Escape key is pressed
                # We don't need to actually hide the inspect popup here because
                # it will be hidden already by the Escape action
                self.inspect_mode = False
        elif event_type == QEvent.MouseButtonPress or event_type == QEvent.MouseButtonRelease:
            # Trigger inspecting the currently hovered widget when the left mouse button is clicked
            # Don't continue processing this event
            if self.inspect_mode and event.button() == Qt.LeftButton:
                # Only trigger the inspect on the click release, but we want to also eat the press
                # event so that the widget we clicked on isn't stuck in a weird state (e.g. thinks its being dragged)
                # Also hide the inspect popup since it won't be hidden automatically by the mouse click since we are
                # consuming the event
                if event_type == event_type == QEvent.MouseButtonRelease:
                    self.inspect_popup.hide()
                    self.inspect_widget()
                return True

        # Pass every event through
        return False

    def build_tree(self, obj, parent_tree):
        if len(obj.children()) == 0:
            return
        for child in obj.children():
            object_type = type(child).__name__
            object_name = child.objectName()
            text = icon_text = title = window_title = geometry_str = classes = "(N/A)"
            if isinstance(child, QtGui.QWindow):
                title = child.title()
            if isinstance(child, QAction):
                text = child.text()
                icon_text = child.iconText()
            if isinstance(child, QWidget):
                window_title = child.windowTitle()
                if not (child.property("class") == ""):
                    classes = child.property("class")
            if isinstance(child, QAbstractButton):
                text = child.text()
            
            
            # Keep track of the pointer address for this object so we can search for it later
            pointer_address = str(int(getCppPointer(child)[0]))

            # Some objects might not have a geometry (e.g. actions, generic qobjects)
            if hasattr(child, 'geometry'):
                geometry_rect = child.geometry()
                geometry_str = "x: {x}, y: {y}, width: {width}, height: {height}".format(x=geometry_rect.x(), y=geometry_rect.y(), width=geometry_rect.width(), height=geometry_rect.height())

            child_tree = QTreeWidgetItem([object_type, object_name, text, icon_text, title, window_title, classes, pointer_address, geometry_str])
            if isinstance(parent_tree, QTreeWidget):
                parent_tree.addTopLevelItem(child_tree)
            else:
                parent_tree.addChild(child_tree)
            self.build_tree(child, child_tree)

    def update_hovered_widget_popup(self):
        if self.inspect_mode and self.hovered_widget:
            if not self.inspect_popup.isVisible():
                self.inspect_popup.show()

            self.inspect_popup.update_widget(self.hovered_widget)
        else:
            self.inspect_popup.hide()

    def on_inspect_clicked(self):
        self.inspect_mode = True
        self.update_hovered_widget_popup()

    def on_tree_widget_selection_changed(self):
        selected_items = self.tree_widget.selectedItems()

        # If nothing is selected, then switch the stacked layout back to 0
        # to show the message
        if not selected_items:
            self.stacked_layout.setCurrentIndex(0)
            return

        # Update the selected widget properties and switch to the 1 index in
        # the stacked layout so that all the rows will be visible
        item = selected_items[0]
        self.name_value.setText(item.text(self.tree_widget_columns.index("OBJECT NAME")))
        self.type_value.setText(item.text(self.tree_widget_columns.index("TYPE")))
        self.geometry_value.setText(item.text(self.tree_widget_columns.index("GEOMETRY")))
        self.text_value.setText(item.text(self.tree_widget_columns.index("TEXT")))
        self.icon_text_value.setText(item.text(self.tree_widget_columns.index("ICONTEXT")))
        self.title_value.setText(item.text(self.tree_widget_columns.index("TITLE")))
        self.window_title_value.setText(item.text(self.tree_widget_columns.index("WINDOW_TITLE")))
        self.classes_value.setText(item.text(self.tree_widget_columns.index("CLASSES")))
        self.stacked_layout.setCurrentIndex(1)

    def inspect_widget(self):
        self.inspect_mode = False

        # Find the tree widget item that matches our hovered widget, and then set it as the current item
        # so that the tree widget will scroll to it, expand it, and select it
        widget_pointer_address = str(int(getCppPointer(self.hovered_widget)[0]))
        pointer_address_column = self.tree_widget_columns.index("POINTER_ADDRESS")
        items = self.tree_widget.findItems(widget_pointer_address, Qt.MatchFixedString | Qt.MatchRecursive, pointer_address_column)
        if items:
            item = items[0]
            self.tree_widget.clearSelection()
            self.tree_widget.setCurrentItem(item)
        else:
            print("Unable to find widget")

def get_object_tree(parent, obj=None):
    """
    Returns the parent/child hierarchy for the given obj (QObject)
    parent: Parent for the dialog that is created
    obj: Root object for the tree to be built.
    returns: QTreeWidget object starting with the root element obj.
    """
    w = ObjectTreeDialog(parent, obj)
    w.resize(1000, 500)

    return w
    
if __name__ == "__main__":
    # Get our Editor main window
    params = azlmbr.qt.QtForPythonRequestBus(azlmbr.bus.Broadcast, "GetQtBootstrapParameters")
    editor_id = QtWidgets.QWidget.find(params.mainWindowId)
    editor_main_window = wrapInstance(int(getCppPointer(editor_id)[0]), QtWidgets.QMainWindow)
    dock_main_window = editor_main_window.findChild(QtWidgets.QMainWindow)

    # Show our object tree visualizer
    object_tree = get_object_tree(dock_main_window)
    object_tree.show()
