"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

"""
C16929882: Required Components
https://testrail.agscollab.com/index.php?/cases/view/16929882
"""

import os
import sys

import azlmbr.legacy.general as general
import Tests.ly_shared.pyside_utils as pyside_utils
import azlmbr.bus as bus
import azlmbr.editor as editor
from azlmbr.entity import EntityId
import azlmbr.entity as entity

import Tests.ly_shared.hydra_editor_utils as hydra
from PySide2 import QtWidgets, QtTest, QtCore
from PySide2.QtCore import Qt
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper

class RequiredComponentsTest(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="required_components: ", args=["level"])

    @pyside_utils.wrap_async
    async def run_test(self):
        """
        Summary:
        Add required components to an entity.

        Expected Behavior:
        1) An entity inspector will show a warning if a required component is missing.
        2) It will allow that component to be added.
        3) The warning will disappear once the component is present..

        Test Steps:
        1) Open level.
        2) Create entity.
        3) Add a tube shape.
        4) Check there is a required component warning and drop down in the inspector.
        5) Click "Add Required Component".
        6) Check that a list of valid components has appeared.
        7) Click "Spline in the list.
        8) Check that a spline component is added to the entity.
        9) Check that the warning drop down is gone.

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """
        def fail_test(msg):
            print(msg)
            print("Test failed.")
            sys.exit()

        def get_component_type_id(component_type_name):
            # Get Component Types for required component.
            type_ids_list = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType',
                                                         [component_type_name], entity.EntityType().Game)

            if len(type_ids_list) == 0:
                print("Type Ids List returned incorrectly.")
                return False, 0

            return True, type_ids_list[0]

        def find_component_inspector(inspector_list, component_name):
            for item in inspector_list:
                title = item.findChild(QtWidgets.QLabel, "Title")
                if title is None:
                    continue
                print("Checking " + title.text())
                if title.text() == component_name:
                    return item
            return None

        async def add_spine_to_tube_shape(button):
            pyside_utils.click_button_async(button)
            list_widget = await pyside_utils.wait_for_popup_widget()

            tree = pyside_utils.find_child_by_pattern(list_widget, "Tree")
            QtTest.QTest.keyClick(tree, Qt.Key_Down, Qt.NoModifier)

            while tree.indexBelow(tree.currentIndex()) != QtCore.QModelIndex():
                if tree.currentIndex().data(Qt.DisplayRole) == "Shape":
                    # Expand the Shape category
                    QtTest.QTest.keyClick(tree, Qt.Key_Right, Qt.NoModifier)
                    QtTest.QTest.keyClick(tree, Qt.Key_Right, Qt.NoModifier)
                if tree.currentIndex().data(Qt.DisplayRole) == "Spline":
                    # Add this component type
                    QtTest.QTest.keyClick(tree, Qt.Key_Enter, Qt.NoModifier)
                    general.idle_wait(0.0)
                    return

                QtTest.QTest.keyClick(tree, Qt.Key_Down, Qt.NoModifier)


        # Create level
        create_level_result = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )

        if not create_level_result:
            fail_test("New level failed.")

        EditorTestHelper.after_level_load(self)

        # Ensure the inspector window is open.
        general.open_pane('Entity Inspector')

        # Create an entity
        new_entity_id = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', EntityId())

        # Select the entity.
        editor.ToolsApplicationRequestBus(bus.Broadcast, 'SetSelectedEntities', [new_entity_id])

        # Add a Tube Shape Component
        editor_window = pyside_utils.get_editor_main_window()
        entity_inspector = editor_window.findChild(QtWidgets.QDockWidget, "Entity Inspector")

        tube_component = hydra.add_component("Tube Shape", new_entity_id)
        general.idle_wait(0.0)
        result, tube_component_id = get_component_type_id("Tube Shape")
        if not result:
            fail_test("Failed to find Tube Shape id.")

        result, spline_component_id = get_component_type_id("Spline")
        if not result:
            fail_test("Failed to find Spline id.")

        if not editor.EditorComponentAPIBus(bus.Broadcast, "HasComponentOfType", new_entity_id, tube_component_id):
            fail_test("Failed to add Tube Shape to entity.")

        is_enabled = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', tube_component)
        if is_enabled:
            fail_test("Tube is enabled when first created.")

        component_list = pyside_utils.find_child_by_hierarchy(entity_inspector, ..., {"objectName": "m_componentListContents"})
        if component_list is None:
            fail_test("Failed to find component list.")

        # Find the add required component button. It's inside a lot of anonymous frames,
        # so find the header_frame and work down from its parent.
        tube_frame = find_component_inspector(component_list.children(), "Tube Shape")
        if tube_frame is None:
            fail_test("Failed to find tube shape inspector.")

        header_frame = pyside_utils.find_child_by_hierarchy(tube_frame, ..., {"objectName": "HeaderFrame"})
        if header_frame is None:
            fail_test("Unable to find tube header frame")

        container_frame = header_frame.parent()
        if container_frame is None:
            fail_test("Unable to find tube container frame")

        add_button = container_frame.findChild(QtWidgets.QPushButton)
        if add_button is None:
            fail_test("Unable to find add required component button")

        # Check there is no spline component yet
        if editor.EditorComponentAPIBus(bus.Broadcast, "HasComponentOfType", new_entity_id, spline_component_id):
            fail_test("Spline component already exists.")

        await add_spine_to_tube_shape(add_button)

        if not editor.EditorComponentAPIBus(bus.Broadcast, "HasComponentOfType", new_entity_id, spline_component_id):
            fail_test("Failed to add Spline component to entity.")

        is_enabled = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', tube_component)
        if not is_enabled:
            fail_test("Tube is not enabled.")

        # Check that the missing component warning is gone.
        editor.ToolsApplicationRequestBus(bus.Broadcast, 'SetSelectedEntities', [new_entity_id])

        component_list = pyside_utils.find_child_by_hierarchy(entity_inspector, ...,
                                                              {"objectName": "m_componentListContents"})
        if component_list is None:
            fail_test("Failed to find component list.")

        tube_frame = find_component_inspector(component_list.children(), "Tube Shape")
        if tube_frame is None:
            fail_test("Failed to find tube shape inspector.")

        header_frame = pyside_utils.find_child_by_hierarchy(tube_frame, ..., {"objectName": "HeaderFrame"})
        if header_frame is not None:
            fail_test("Missing component warning still exists")

        print("Required Component test successful.")


test = RequiredComponentsTest()
test.run()