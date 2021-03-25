"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

C6376081: Basic Function: Docked/Undocked Tools
https://testrail.agscollab.com/index.php?/cases/view/6376081
"""

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as entity
import azlmbr.legacy.general as general

from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper
import Tests.ly_shared.pyside_utils as pyside_utils

from PySide2 import QtCore, QtWidgets, QtTest


class TestBasicDockedTools(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="basic_docked_tools: ", args=["level"])

    @pyside_utils.wrap_async
    async def run_test(self):
        """
        Summary:
        Test that tools still work as expected when docked together.

        Expected Behavior:
        Multiple tools can be docked together.
        Tools function while docked together and the main editor responds appropriately.

        Test Steps:
        1) Open the tools and dock them together in a floating tabbed widget.
        2) Perform actions in the docked tools to verify they still work as expected.
            2.1) Select the Entity Outliner in the floating window.
            2.2) Select an Entity in the Entity Outliner.
            2.3) Select the Entity Inspector in the floating window.
            2.4) Change the name of the selected Entity via the Entity Inspector.
            2.5) Select the Console inside the floating window.
            2.6) Send a console command.
            2.7) Check the Editor to verify all changes were made.

        :return: None
        """

        # Create a level since we are going to be dealing with an Entity.
        self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )

        # Make sure the Entity Outliner, Entity Inspector and Console tools are open
        general.open_pane("Entity Outliner (PREVIEW)")
        general.open_pane("Entity Inspector")
        general.open_pane("Console")

        # Create an Entity to test with
        entity_original_name = 'MyTestEntity'
        entity_id = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', entity.EntityId())
        editor.EditorEntityAPIBus(bus.Event, 'SetName', entity_id, entity_original_name)

        editor_window = pyside_utils.get_editor_main_window()
        entity_outliner = editor_window.findChild(QtWidgets.QDockWidget, "Entity Outliner (PREVIEW)")

        # 1) Open the tools and dock them together in a floating tabbed widget.
        # We drag/drop it over the viewport since it doesn't allow docking, so this will undock it
        render_overlay = editor_window.findChild(QtWidgets.QWidget, "renderOverlay")
        pyside_utils.drag_and_drop(entity_outliner, render_overlay)

        # We need to grab a new reference to the Entity Outliner QDockWidget because when it gets moved
        # to the floating window, its parent changes so the wrapped intance we had becomes invalid
        entity_outliner = editor_window.findChild(QtWidgets.QDockWidget, "Entity Outliner (PREVIEW)")

        # Dock the Entity Inspector tabbed with the floating Entity Outliner
        entity_inspector = editor_window.findChild(QtWidgets.QDockWidget, "Entity Inspector")
        pyside_utils.drag_and_drop(entity_inspector, entity_outliner)

        # We need to grab a new reference to the Entity Inspector QDockWidget because when it gets moved
        # to the floating window, its parent changes so the wrapped intance we had becomes invalid
        entity_inspector = editor_window.findChild(QtWidgets.QDockWidget, "Entity Inspector")

        # Dock the Console tabbed with the floating Entity Inspector
        console = editor_window.findChild(QtWidgets.QDockWidget, "Console")
        pyside_utils.drag_and_drop(console, entity_inspector)

        # Check to ensure all the tools are parented to the same QStackedWidget
        def check_all_panes_tabbed():
            entity_inspector = editor_window.findChild(QtWidgets.QDockWidget, "Entity Inspector")
            entity_outliner = editor_window.findChild(QtWidgets.QDockWidget, "Entity Outliner (PREVIEW)")
            console = editor_window.findChild(QtWidgets.QDockWidget, "Console")
            entity_inspector_parent = entity_inspector.parentWidget()
            entity_outliner_parent = entity_outliner.parentWidget()
            console_parent = console.parentWidget()
            print(f"Entity Inspector parent = {entity_inspector_parent}, Entity Outliner parent = {entity_outliner_parent}, Console parent = {console_parent}")
            return isinstance(entity_inspector_parent, QtWidgets.QStackedWidget) and (entity_inspector_parent == entity_outliner_parent) and (entity_outliner_parent == console_parent)

        success = await pyside_utils.wait_for(check_all_panes_tabbed, timeout=3.0)
        if success:
            print("The tools are all docked together in a tabbed widget")

        # 2.1,2) Select an Entity in the Entity Outliner.
        entity_inspector = editor_window.findChild(QtWidgets.QDockWidget, "Entity Inspector")
        entity_outliner = editor_window.findChild(QtWidgets.QDockWidget, "Entity Outliner (PREVIEW)")
        console = editor_window.findChild(QtWidgets.QDockWidget, "Console")
        object_tree = entity_outliner.findChild(QtWidgets.QTreeView, "m_objectTree")
        test_entity_index = pyside_utils.find_child_by_pattern(object_tree, entity_original_name)
        object_tree.clearSelection()
        object_tree.setCurrentIndex(test_entity_index)
        selected_object_names = general.get_names_of_selected_objects()
        if len(selected_object_names) == 1 and selected_object_names[0] == entity_original_name:
            print("Entity Outliner works when docked, can select an Entity")

        # 2.3,4) Change the name of the selected Entity via the Entity Inspector.
        entity_inspector_name_field = entity_inspector.findChild(QtWidgets.QLineEdit, "m_entityNameEditor")
        expected_new_name = "DifferentName"
        entity_inspector_name_field.setText(expected_new_name)
        QtTest.QTest.keyClick(entity_inspector_name_field, QtCore.Qt.Key_Enter)
        entity_new_name = editor.EditorEntityInfoRequestBus(bus.Event, "GetName", entity_id)
        if entity_new_name == expected_new_name:
            print(f"Entity Inspector works when docked, Entity name changed to {entity_new_name}")

        # 2.5,6) Send a console command.
        console_line_edit = console.findChild(QtWidgets.QLineEdit, "lineEdit")
        console_line_edit.setText("e_Vegetation=1")
        QtTest.QTest.keyClick(console_line_edit, QtCore.Qt.Key_Enter)


test = TestBasicDockedTools()
test.run()
